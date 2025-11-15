#pragma once
#include <grpcpp/grpcpp.h>

#include "blob_relay_streaming.grpc.pb.h"
#include "blob_relay_streaming.pb.h"
#include "session_manager.h"
   
namespace data_relay_grpc::blob_relay {

using data_relay_grpc::blob_relay::proto::BlobRelayStreaming;
using data_relay_grpc::blob_relay::proto::PutStreamingRequest;
using data_relay_grpc::blob_relay::proto::PutStreamingResponse;
using data_relay_grpc::blob_relay::proto::GetStreamingRequest;
using data_relay_grpc::blob_relay::proto::GetStreamingResponse;

class streaming_service final : public BlobRelayStreaming::Service {
public:
    streaming_service(blob_session_manager&, std::size_t);
    ~streaming_service() override = default;

    streaming_service(const streaming_service&) = delete;
    streaming_service& operator=(const streaming_service&) = delete;
    streaming_service(streaming_service&&) = delete;
    streaming_service& operator=(streaming_service&&) = delete;

    ::grpc::Status Get(::grpc::ServerContext* context,
                       const GetStreamingRequest* request,
                       ::grpc::ServerWriter< GetStreamingResponse>* writer) override;

    ::grpc::Status Put(::grpc::ServerContext* context,
                       ::grpc::ServerReader< PutStreamingRequest>* reader,
                       PutStreamingResponse* response) override;

private:
    blob_session_manager& session_manager_;
    std::size_t chunk_size_;
    constexpr static std::uint64_t SESSION_STORAGE_ID = 0;
    constexpr static std::uint64_t LIMESTONE_BLOB_STORE = 1;
};

} // namespace data_relay_grpc::blob_relay
