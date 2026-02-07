#pragma once
#include <grpcpp/grpcpp.h>

#include <data_relay_grpc/common/detail/session_manager.h>
#include "data_relay_grpc/proto/blob_relay/blob_relay_local.grpc.pb.h"
#include "data_relay_grpc/proto/blob_relay/blob_relay_local.pb.h"

namespace data_relay_grpc::blob_relay {

using data_relay_grpc::proto::blob_relay::blob_relay_local::BlobRelayLocal;
using data_relay_grpc::proto::blob_relay::blob_relay_local::PutLocalRequest;
using data_relay_grpc::proto::blob_relay::blob_relay_local::PutLocalResponse;
using data_relay_grpc::proto::blob_relay::blob_relay_local::GetLocalRequest;
using data_relay_grpc::proto::blob_relay::blob_relay_local::GetLocalResponse;

class local_service final : public BlobRelayLocal::Service {
  public:
    explicit local_service(common::detail::blob_session_manager& server);
    ~local_service() override = default;

    local_service(const local_service&) = delete;
    local_service& operator=(const local_service&) = delete;
    local_service(local_service&&) = delete;
    local_service& operator=(local_service&&) = delete;

    ::grpc::Status Get(::grpc::ServerContext* context,
                       const GetLocalRequest* request,
                       GetLocalResponse* response) override;

    ::grpc::Status Put(::grpc::ServerContext* context,
                       const PutLocalRequest* request,
                       PutLocalResponse* response) override;

private:
    common::detail::blob_session_manager& session_manager_;
};

} // namespace data_relay_grpc::blob_relay
