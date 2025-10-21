#pragma once

#include <gtest/gtest.h>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>

#include <chrono>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "service/ping_service.h"

namespace data_relay_grpc::grpc {

class grpc_server_test_base : public ::testing::Test {
protected:
    std::unique_ptr<::grpc::Server> server_;
    std::string server_address_;
    std::unique_ptr<data_relay_grpc::grpc::service::ping_service> ping_service_{};
    std::unique_ptr<::grpc::Service> service_{};
    std::function<void(::grpc::ServerBuilder&)> service_handler_{};

    void set_service_handler(std::function<void(::grpc::ServerBuilder&)> f) {
        service_handler_ = std::move(f);
    }

    void start_server() {
        ::grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address_, ::grpc::InsecureServerCredentials());
        ping_service_ = std::make_unique<data_relay_grpc::grpc::service::ping_service>();
        builder.RegisterService(ping_service_.get());
        if (service_handler_) {
            service_handler_(builder);
        }
        server_ = builder.BuildAndStart();
        ASSERT_TRUE(server_ != nullptr);
        wait_for_server_ready();
    }

    // Wait until the server is ready (using ping_service)
    void wait_for_server_ready() {
        constexpr int max_attempts = 50;
        constexpr int wait_millis = 10;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            if (is_server_ready()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_millis));
        }
        throw std::runtime_error("gRPC server did not become ready in time");
    }

    void SetUp() override {
        server_address_ = find_available_address();
    }

    void TearDown() override {
        if (server_) {
            server_->Shutdown();
            server_.reset();
        }
    }

    // Use ping_service to check if server is ready
    virtual bool is_server_ready() {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        data_relay_grpc::grpc::proto::PingService::Stub stub(channel);
        ::grpc::ClientContext context;
        data_relay_grpc::grpc::proto::PingRequest req;
        data_relay_grpc::grpc::proto::PingResponse resp;
        auto status = stub.Ping(&context, req, &resp);
        return status.ok();
    }

    // Find and return an available port number in the range 50000-50200
    std::string find_available_address() {
        for (int port = 50000; port <= 50200; ++port) {
            std::string address = "127.0.0.1:" + std::to_string(port);
            if (is_port_available(port)) {
                return address;
            }
        }
        throw std::runtime_error("No available port found in range 50000-50200");
    }

    // Check if the port is available
    bool is_port_available(int port) {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        int result = ::bind(sock, (struct sockaddr*)&addr, sizeof(addr));
        ::close(sock);
        return result == 0;
    }
};

} // namespace
