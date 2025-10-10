#include <fstream>

#include <data-relay-grpc/blob_relay/blob_session_manager.h>
#include <data-relay-grpc/blob_relay/local_service_impl.h>
#include "utils.h"

namespace data_relay_grpc::blob_relay {

BlobRelayLocalImpl::BlobRelayLocalImpl(blob_session_manager& session_manager)
    : session_manager_(session_manager) {
}

::grpc::Status BlobRelayLocalImpl::Get([[maybe_unused]] ::grpc::ServerContext* context,
                                       const GetLocalRequest* request,
                                       GetLocalResponse* response) {
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

        response->mutable_data()->set_path(session_manager_.get_path(blob_id));
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    }
    
    return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "the session has no transaction");
}

::grpc::Status BlobRelayLocalImpl::Put([[maybe_unused]] ::grpc::ServerContext* context,
                                       const ::data_relay_grpc::blob_relay::PutLocalRequest* request,
                                       PutLocalResponse* response) {
    if (!check_api_version(request->api_version())) {
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "inappropriate message version");
    }

    auto& session = session_manager_.get_session(request->session_id());
    auto pair = session.create_blob_file();
    std::filesystem::copy(std::filesystem::path(request->data().path()), pair.second);
    auto* blob = response->mutable_blob();
    blob->set_storage_id(0);
    blob->set_object_id(pair.first);
    return ::grpc::Status(::grpc::StatusCode::OK, "");
}

} // namespace data_relay_grpc::blob_relay
