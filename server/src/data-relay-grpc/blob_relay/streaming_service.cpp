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
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, "the requested API version is not compatible");
    }

    try {
        auto& session_impl = session_manager_.get_session_impl(request->session_id());
        if (auto transaction_id_opt = session_impl.get_transaction_id(); transaction_id_opt) {
            blob_session::transaction_id_type transaction_id = transaction_id_opt.value();
            blob_session::blob_id_type blob_id = request->blob().object_id();

            blob_session::blob_tag_type tag = session_manager_.get_tag(blob_id, transaction_id);
            if (tag != request->blob().tag()) {
                return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "tag mismatch");
            }

            GetStreamingResponse response{};
            auto path = session_manager_.get_path(blob_id);
            if (std::filesystem::exists(path)) {
                std::ifstream ifs(session_manager_.get_path(blob_id));
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
                try {
                    std::filesystem::remove(path);
                } catch (std::exception &ex) {
                    return ::grpc::Status(::grpc::StatusCode::INTERNAL, ex.what());
                }
                return ::grpc::Status(::grpc::StatusCode::OK, "");
            } else {
                return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "there is no blob entry or no file corresponding to the blob id");
            }
        }
        return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED, "the session has no transaction");
    } catch (std::out_of_range &ex) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
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
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, "the requested API version is not compatible");
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
