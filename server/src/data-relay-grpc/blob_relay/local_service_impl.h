#pragma once
#include <grpcpp/grpcpp.h>

#include <data-relay-grpc/blob_relay/blob_session_manager.h>
#include "blob_relay_local.grpc.pb.h"
#include "blob_relay_local.pb.h"

namespace data_relay_grpc::blob_relay {

class BlobRelayLocalImpl final : public BlobRelayLocal::Service {
  public:
    explicit BlobRelayLocalImpl(blob_session_manager& server);
    ~BlobRelayLocalImpl() override = default;

    BlobRelayLocalImpl(const BlobRelayLocalImpl&) = delete;
    BlobRelayLocalImpl& operator=(const BlobRelayLocalImpl&) = delete;
    BlobRelayLocalImpl(BlobRelayLocalImpl&&) = delete;
    BlobRelayLocalImpl& operator=(BlobRelayLocalImpl&&) = delete;

    ::grpc::Status Get(::grpc::ServerContext* context,
                       const GetLocalRequest* request,
                       GetLocalResponse* response) override;

    ::grpc::Status Put(::grpc::ServerContext* context,
                       const ::data_relay_grpc::blob_relay::PutLocalRequest* request,
                       PutLocalResponse* response) override;

private:
    blob_session_manager& session_manager_;
};

} // namespace data_relay_grpc::blob_relay
