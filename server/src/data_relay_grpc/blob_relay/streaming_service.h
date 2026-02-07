#pragma once
#include <grpcpp/grpcpp.h>

#include <string_view>

#include <data_relay_grpc/common/detail/session_manager.h>
#include "data_relay_grpc/proto/blob_relay/blob_relay_streaming.grpc.pb.h"
#include "data_relay_grpc/proto/blob_relay/blob_relay_streaming.pb.h"
   
namespace data_relay_grpc::blob_relay {

using data_relay_grpc::proto::blob_relay::blob_relay_streaming::BlobRelayStreaming;
using data_relay_grpc::proto::blob_relay::blob_relay_streaming::PutStreamingRequest;
using data_relay_grpc::proto::blob_relay::blob_relay_streaming::PutStreamingRequest_Metadata;
using data_relay_grpc::proto::blob_relay::blob_relay_streaming::PutStreamingResponse;
using data_relay_grpc::proto::blob_relay::blob_relay_streaming::GetStreamingRequest;
using data_relay_grpc::proto::blob_relay::blob_relay_streaming::GetStreamingResponse;
using data_relay_grpc::proto::blob_relay::blob_relay_streaming::GetStreamingResponse_Metadata;

class streaming_service final : public BlobRelayStreaming::Service {
public:
    streaming_service(common::detail::blob_session_manager& session_manager, std::size_t chunk_size);
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
    common::detail::blob_session_manager& session_manager_;
    std::size_t chunk_size_;
    constexpr static std::uint64_t SESSION_STORAGE_ID = 0;
    constexpr static std::uint64_t LIMESTONE_BLOB_STORE = 1;

    static std::string_view storage_name(std::uint64_t sid) {
        using namespace std::string_view_literals;
        return sid == SESSION_STORAGE_ID ? "session storage"sv : "limestone blob store"sv;
    }
};

} // namespace data_relay_grpc::blob_relay
