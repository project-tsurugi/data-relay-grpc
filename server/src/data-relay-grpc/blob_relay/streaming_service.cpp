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
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "inappropriate message version");
    }

    auto& session = session_manager_.get_session(request->session_id());
    if (auto transaction_id_opt = session.get_transaction_id(); transaction_id_opt) {
        blob_session::transaction_id_type transaction_id = transaction_id_opt.value();
        blob_session::blob_id_type blob_id = request->blob().object_id();

        blob_session::blob_tag_type tag = session_manager_.get_tag(transaction_id, blob_id);

        if (tag != request->blob().tag()) {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "can not find blob with the tag given");
        }

        GetStreamingResponse response{};
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
    }
    return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "the session has no transaction");
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
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "inappropriate message version");
    }
    auto& session = session_manager_.get_session(request.metadata().session_id());
    auto pair = session.create_blob_file();
    blob_session::blob_id_type blob_id = pair.first;

    std::ofstream blob_file(pair.second);
    while (reader->Read(&request)) {
        if (request.payload_case() != PutStreamingRequest::PayloadCase::kChunk) {
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "A subsequent requests is not chunk");
        }
        auto& chunk = request.chunk();
        blob_file.write(chunk.data(), chunk.size());
    }

    auto* blob = response->mutable_blob();
    blob->set_storage_id(0);
    blob->set_object_id(blob_id);
    return ::grpc::Status(::grpc::StatusCode::OK, "");
}

} // namespace data_relay_grpc::blob_relay
