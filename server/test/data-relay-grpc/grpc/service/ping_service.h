#pragma once

#include "ping_service.grpc.pb.h"

namespace data_relay_grpc::grpc::service {

using data_relay_grpc::grpc::proto::PingService;
using data_relay_grpc::grpc::proto::PingRequest;
using data_relay_grpc::grpc::proto::PingResponse;

class ping_service final : public PingService::Service {
public:
    ::grpc::Status Ping(::grpc::ServerContext* , const PingRequest* request, PingResponse* response) override {
        (void)request;
        (void)response;
        return ::grpc::Status::OK;
    }
};

} // namespace
