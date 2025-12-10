#include <fstream>

#include "session_manager.h"
#include "streaming_service.h"
#include "utils.h"

namespace data_relay_grpc::blob_relay {

streaming_service::streaming_service(blob_session_manager& session_manager, std::size_t chunk_size)
    : session_manager_(session_manager), chunk_size_(chunk_size) {
}

::grpc::Status streaming_service::Get(::grpc::ServerContext*,
                                      const GetStreamingRequest* request,
                                      ::grpc::ServerWriter< GetStreamingResponse>* writer) {
    if (!check_api_version(request->api_version())) {
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, api_version_error_message(request->api_version()));
    }

    try {
        blob_session::session_id_type session_id{};
        blob_session::transaction_id_type transaction_id{};
        blob_session::blob_id_type blob_id = request->blob().object_id();
        if (request->context_id_case() == GetStreamingRequest::ContextIdCase::kSessionId) {
            session_id = request->session_id();
        } else if (request->context_id_case() == GetStreamingRequest::ContextIdCase::kTransactionId) {
            transaction_id = request->transaction_id();
            session_id = session_manager_.get_session_id(transaction_id);
        } else {
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "content_id is neither session_id nor transaction_id");
        }

        auto& session_impl = session_manager_.get_session_impl(session_id);
        if (request->context_id_case() == GetStreamingRequest::ContextIdCase::kTransactionId) {
            if (auto transaction_id_opt = session_impl.get_transaction_id(); transaction_id_opt) {
                if (transaction_id_opt.value() != transaction_id) {
                    return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "transaction_id does not match with that of the session");
                }
            } else {
                return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "the session has no transaction");
            }
        }

        blob_session::blob_path_type path{};
        auto storage_id = request->blob().storage_id();
        if (storage_id == SESSION_STORAGE_ID) {
            if (auto path_opt = session_impl.find(blob_id); path_opt) {
                path = path_opt.value();
            } else {
                return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "can not find the blob data by the blob_id given");
            }
        } else if (storage_id == LIMESTONE_BLOB_STORE) {
            path = session_manager_.get_path(blob_id);
        } else {
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "storage_id is neither session store nor limestone blob store");
        }

        // should be done after confirming the blog's existence
        if (session_impl.compute_tag(blob_id) != request->blob().tag()) {
            if (!session_manager_.dev_accept_mock_tag() || request->blob().tag() != blob_session_manager::MOCK_TAG) {
                return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "the given tag does not match the desiring value");
            }
        }

        GetStreamingResponse response{};
        if (std::filesystem::exists(path)) {
            std::ifstream ifs(path);
            std::string s{};
            s.resize(chunk_size_);
            while (true) {
                ifs.read(s.data(), s.length());
                auto size = ifs.gcount();
                if (size == 0) {
                    response.clear_chunk();
                    writer->Write(response);
                    return ::grpc::Status(::grpc::StatusCode::OK, "");
                }
                response.set_chunk(s.data(), size);
                writer->Write(response);
            }
            return ::grpc::Status(::grpc::StatusCode::OK, "");
        } else {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "an error occurred while reading the blob file");
        }
    } catch (std::out_of_range &ex) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
    } catch (std::exception &ex) {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
    }
}

::grpc::Status streaming_service::Put(::grpc::ServerContext*,
                                      ::grpc::ServerReader< PutStreamingRequest>* reader,
                                      PutStreamingResponse* response) {
    PutStreamingRequest request;
    if (!reader->Read(&request)) {
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "no request");
    }

    if (request.payload_case() != PutStreamingRequest::PayloadCase::kMetadata) {
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "the first request is not metadata");
    }
    if (!check_api_version(request.metadata().api_version())) {
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, api_version_error_message(request.metadata().api_version()));
    }
    try {
        auto& session_impl = session_manager_.get_session_impl(request.metadata().session_id());
        auto pair = session_impl.create_blob_file();
        blob_session::blob_id_type blob_id = pair.first;

        std::ofstream blob_file(pair.second);
        if (!blob_file.is_open()) {
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, "cannot open the file to write the blob to");
        }
        bool has_chunk{};
        while (reader->Read(&request)) {
            if (request.payload_case() != PutStreamingRequest::PayloadCase::kChunk) {
                return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "A subsequent requests is not chunk");
            }
            auto& chunk = request.chunk();
            has_chunk = true;
            if (!session_impl.reserve_session_store(blob_id, chunk.size())) {
                blob_file.close();
                session_impl.delete_blob_file(blob_id);
                return ::grpc::Status(::grpc::StatusCode::RESOURCE_EXHAUSTED, "session storage usage has reached its limit");
            }
            blob_file.write(chunk.data(), chunk.size());
        }
        blob_file.close();
        if (has_chunk) {
            auto* blob = response->mutable_blob();
            blob->set_storage_id(0);
            blob->set_object_id(blob_id);
            return ::grpc::Status(::grpc::StatusCode::OK, "");
        } else {
            std::filesystem::remove(pair.second);
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "no chunk has been sent");
        }
    } catch (std::out_of_range &ex) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
    }
}

} // namespace data_relay_grpc::blob_relay
