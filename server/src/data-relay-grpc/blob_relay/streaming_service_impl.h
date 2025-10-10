#pragma once
#include <grpcpp/grpcpp.h>

#include <data-relay-grpc/blob_relay/blob_session_manager.h>
#include "blob_relay_streaming.grpc.pb.h"
#include "blob_relay_streaming.pb.h"
   
namespace data_relay_grpc::blob_relay {

class BlobRelayStreamingImpl final : public BlobRelayStreaming::Service {
  public:
    BlobRelayStreamingImpl(blob_session_manager&, std::size_t);
    ~BlobRelayStreamingImpl() override = default;

    BlobRelayStreamingImpl(const BlobRelayStreamingImpl&) = delete;
    BlobRelayStreamingImpl& operator=(const BlobRelayStreamingImpl&) = delete;
    BlobRelayStreamingImpl(BlobRelayStreamingImpl&&) = delete;
    BlobRelayStreamingImpl& operator=(BlobRelayStreamingImpl&&) = delete;

    ::grpc::Status Get(::grpc::ServerContext* context,
                       const GetStreamingRequest* request,
                       ::grpc::ServerWriter< GetStreamingResponse>* writer) override;

    ::grpc::Status Put(::grpc::ServerContext* context,
                       ::grpc::ServerReader< PutStreamingRequest>* reader,
                       PutStreamingResponse* response) override;

private:
    blob_session_manager& session_manager_;
    std::size_t chunk_size_;
};

} // namespace data_relay_grpc::blob_relay
