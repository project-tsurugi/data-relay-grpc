#include <fstream>
#include <optional>

#include "session_manager.h"
#include "streaming_service.h"

#include <glog/logging.h>
#include "data-relay-grpc/logging_helper.h"
#include "data-relay-grpc/logging.h"

#include "utils.h"

namespace data_relay_grpc::blob_relay {

streaming_service::streaming_service(blob_session_manager& session_manager, std::size_t chunk_size)
    : session_manager_(session_manager), chunk_size_(chunk_size) {
}

::grpc::Status streaming_service::Get(::grpc::ServerContext*,
                                      const GetStreamingRequest* request,
                                      ::grpc::ServerWriter< GetStreamingResponse>* writer) {
    if (!check_api_version(request->api_version())) {
        VLOG_LP(log_debug) << "finishes with UNAVAILABLE";
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, api_version_error_message(request->api_version()));
    }

    try {
        blob_session::session_id_type session_id{};
        blob_session::transaction_id_type transaction_id{};
        blob_session::blob_id_type blob_id = request->blob().object_id();
        blob_session::blob_tag_type blob_tag = request->blob().tag();
        auto storage_id = request->blob().storage_id();
        if (request->context_id_case() == GetStreamingRequest::ContextIdCase::kSessionId) {
            session_id = request->session_id();
            VLOG_LP(log_debug) << "accepted request: blob_id = " <<  blob_id << " of " << storage_name(storage_id) << ", session_id = " << session_id << ", tag = " << blob_tag;
        } else if (request->context_id_case() == GetStreamingRequest::ContextIdCase::kTransactionId) {
            transaction_id = request->transaction_id();
            session_id = session_manager_.get_session_id(transaction_id);
            VLOG_LP(log_debug) << "accepted request: blob_id = " <<  blob_id << " of " << storage_name(storage_id) << ", transaction_id = " << transaction_id << ", session_id = " << session_id;
        } else {
            VLOG_LP(log_debug) << "finishes with INVALID_ARGUMENT";
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "content_id is neither session_id nor transaction_id");
        }

        auto& session_impl = session_manager_.get_session_impl(session_id);
        if (request->context_id_case() == GetStreamingRequest::ContextIdCase::kTransactionId) {
            if (auto transaction_id_opt = session_impl.get_transaction_id(); transaction_id_opt) {
                if (transaction_id_opt.value() != transaction_id) {
                    VLOG_LP(log_debug) << "finishes with PERMISSION_DENIED";
                    return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "transaction_id does not match with that of the session");
                }
            } else {
                VLOG_LP(log_debug) << "finishes with PERMISSION_DENIED";
                return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "the session has no transaction");
            }
        }

        blob_session::blob_path_type path{};
        if (storage_id == SESSION_STORAGE_ID) {
            if (auto path_opt = session_impl.find(blob_id); path_opt) {
                path = path_opt.value();
                VLOG_LP(log_debug) << "going to send BLOB from sessin storage: path = " << path.string();
            } else {
                VLOG_LP(log_debug) << "finishes with NOT_FOUND";
                return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "can not find the blob data by the blob_id given");
            }
        } else if (storage_id == LIMESTONE_BLOB_STORE) {
            path = session_manager_.get_path(blob_id);
            VLOG_LP(log_debug) << "going to send BLOB from limestone blob store: path = " << path.string();
        } else {
            VLOG_LP(log_debug) << "finishes with INVALID_ARGUMENT";
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "storage_id is neither session store nor limestone blob store");
        }

        // should be done after confirming the blob's existence
        if (session_impl.compute_tag(blob_id) != blob_tag) {
            if (!session_manager_.dev_accept_mock_tag() || blob_tag != blob_session_manager::MOCK_TAG) {
                VLOG_LP(log_debug) << "finishes with PERMISSION_DENIED";
                return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "the given tag does not match the desiring value");
            }
        }

        if (std::filesystem::exists(path)) {
            GetStreamingResponse response{};

            // metadata
            auto* metadata = response.mutable_metadata();
            metadata->set_blob_size(std::filesystem::file_size(path));
            writer->Write(response);

            // chunk
            response.clear_metadata();
            std::ifstream ifs(path);
            std::string s{};
            s.resize(chunk_size_);
            while (true) {
                ifs.read(s.data(), s.length());
                auto size = ifs.gcount();
                if (size == 0) {
                    return ::grpc::Status(::grpc::StatusCode::OK, "");
                }
                response.set_chunk(s.data(), size);
                writer->Write(response);
            }
            VLOG_LP(log_debug) << "finishes normally";
            return ::grpc::Status(::grpc::StatusCode::OK, "");
        } else {
            VLOG_LP(log_debug) << "finishes with NOT_FOUND";
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "an error occurred while reading the blob file");
        }
    } catch (std::out_of_range &ex) {
        VLOG_LP(log_debug) << "finishes with NOT_FOUND";
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
    } catch (std::exception &ex) {
        VLOG_LP(log_debug) << "finishes with INTERNAL";
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
    }
}

::grpc::Status streaming_service::Put(::grpc::ServerContext*,
                                      ::grpc::ServerReader< PutStreamingRequest>* reader,
                                      PutStreamingResponse* response) {
    PutStreamingRequest request;
    if (!reader->Read(&request)) {
        VLOG_LP(log_debug) << "finishes with INVALID_ARGUMENT";
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "no request");
    }

    if (request.payload_case() != PutStreamingRequest::PayloadCase::kMetadata) {
        VLOG_LP(log_debug) << "finishes with INVALID_ARGUMENT";
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "the first request is not metadata");
    }
    auto& metadata = request.metadata();
    if (!check_api_version(metadata.api_version())) {
        VLOG_LP(log_debug) << "finishes with UNAVAILABLE";
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, api_version_error_message(metadata.api_version()));
    }
    std::optional<std::size_t> blob_size_opt{};
    if (metadata.blob_size_opt_case() == PutStreamingRequest_Metadata::BlobSizeOptCase::kBlobSize) {
        blob_size_opt = metadata.blob_size();
    }
    try {
        auto& session_impl = session_manager_.get_session_impl(request.metadata().session_id());
        auto pair = session_impl.create_blob_file();
        blob_session::blob_id_type blob_id = pair.first;
        VLOG_LP(log_debug) << "accepted request: session_id = " << request.metadata().session_id() << ", to be create a blob file with blob_id = " << blob_id << " of session storage";

        std::ofstream blob_file(pair.second);
        if (!blob_file.is_open()) {
            VLOG_LP(log_debug) << "finishes with FAILED_PRECONDITION";
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, "cannot open the file to write the blob to");
        }
        std::size_t total_size{};
        while (reader->Read(&request)) {
            if (request.payload_case() != PutStreamingRequest::PayloadCase::kChunk) {
                VLOG_LP(log_debug) << "finishes with INVALID_ARGUMENT";
                return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "A subsequent requests is not chunk");
            }
            auto& chunk = request.chunk();
            if (!session_impl.reserve_session_store(blob_id, chunk.size())) {
                blob_file.close();
                session_impl.delete_blob_file(blob_id);
                VLOG_LP(log_debug) << "finishes with RESOURCE_EXHAUSTED";
                return ::grpc::Status(::grpc::StatusCode::RESOURCE_EXHAUSTED, "session storage usage has reached its limit");
            }
            blob_file.write(chunk.data(), chunk.size());
            total_size += chunk.size();
        }
        blob_file.close();
        VLOG_LP(log_debug) << "finishes blob file reception, blob_id = " << blob_id;
        if (blob_size_opt) {
            if (blob_size_opt.value() != total_size) {
                std::filesystem::remove(pair.second);
                VLOG_LP(log_debug) << "finishes with INVALID_ARGUMENT";
                return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "the size in the metadata does not match the size of the sent blob");
            }
        }

        auto* blob = response->mutable_blob();
        blob->set_storage_id(SESSION_STORAGE_ID);
        blob->set_object_id(blob_id);
        VLOG_LP(log_debug) << "finishes normally";
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    } catch (std::out_of_range &ex) {
        VLOG_LP(log_debug) << "finishes with NOT_FOUND";
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
    }
}

} // namespace data_relay_grpc::blob_relay
