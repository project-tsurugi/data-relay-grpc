#pragma once
#include <grpcpp/grpcpp.h>

#include "blob_relay_local.grpc.pb.h"
#include "blob_relay_local.pb.h"
#include "session_manager.h"

namespace data_relay_grpc::blob_relay {

using data_relay_grpc::blob_relay::proto::BlobRelayLocal;
using data_relay_grpc::blob_relay::proto::PutLocalRequest;
using data_relay_grpc::blob_relay::proto::PutLocalResponse;
using data_relay_grpc::blob_relay::proto::GetLocalRequest;
using data_relay_grpc::blob_relay::proto::GetLocalResponse;

class local_service final : public BlobRelayLocal::Service {
  public:
    explicit local_service(blob_session_manager& server);
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
    blob_session_manager& session_manager_;
};

} // namespace data_relay_grpc::blob_relay
