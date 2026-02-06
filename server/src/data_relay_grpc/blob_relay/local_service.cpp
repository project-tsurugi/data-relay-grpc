#include <fstream>

#include <data_relay_grpc/common/session.h>
#include "local_service.h"
#include "utils.h"

namespace data_relay_grpc::blob_relay {

using data_relay_grpc::common::blob_session;

local_service::local_service(std::shared_ptr<common::blob_session_manager>& session_manager)
    : session_manager_(session_manager) {
}

::grpc::Status local_service::Get([[maybe_unused]] ::grpc::ServerContext* context,
                                       const GetLocalRequest* request,
                                       GetLocalResponse* response) {
    if (!check_api_version(request->api_version())) {
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, api_version_error_message(request->api_version()));
    }

    auto& session_impl = session_manager_->get_session_impl(request->session_id());
    if (auto transaction_id_opt = session_impl.get_transaction_id(); transaction_id_opt) {
        blob_session::transaction_id_type transaction_id = transaction_id_opt.value();
        blob_session::blob_id_type blob_id = request->blob().object_id();

        blob_session::blob_tag_type tag = session_manager_->get_tag(blob_id, transaction_id);

        if (tag != request->blob().tag()) {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "can not find blob with the tag given");
        }

        response->mutable_data()->set_path(session_manager_->get_path(blob_id));
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    }
    
    return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "the session has no transaction");
}

::grpc::Status local_service::Put([[maybe_unused]] ::grpc::ServerContext* context,
                                  const ::data_relay_grpc::blob_relay::PutLocalRequest* request,
                                  PutLocalResponse* response) {
    if (!check_api_version(request->api_version())) {
        return ::grpc::Status(::grpc::StatusCode::UNAVAILABLE, api_version_error_message(request->api_version()));
    }

    auto& session_impl = session_manager_->get_session_impl(request->session_id());
    auto pair = session_impl.create_blob_file();
    std::filesystem::copy(std::filesystem::path(request->data().path()), pair.second);
    auto* blob = response->mutable_blob();
    blob->set_storage_id(0);
    blob->set_object_id(pair.first);
    return ::grpc::Status(::grpc::StatusCode::OK, "");
}

} // namespace data_relay_grpc::blob_relay
