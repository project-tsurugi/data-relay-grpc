#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <gflags/gflags.h>

#include "blob_relay_smoke_test.grpc.pb.h"

DECLARE_bool(dispose);

namespace data_relay_grpc::blob_relay {

class session {
public:
    session(const std::string& server_address) : server_address_(server_address) {
    }
    ~session() {
        dispose();
    }
    std::size_t session_id() {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        smoke_test::proto::BlobRelaySmokeTestSupport::Stub stub(channel);
        ::grpc::ClientContext context;

        smoke_test::proto::CreateSessionRequest request{};
        smoke_test::proto::CreateSessionResponse response{};
        ::grpc::Status status = stub.CreateSession(&context, request, &response);
        if (status.error_code() !=  ::grpc::StatusCode::OK) {
            throw std::runtime_error(status.error_message());
        }

        return response.session_id();
    }
    void dispose() const {
        if (FLAGS_dispose) {
            auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
            smoke_test::proto::BlobRelaySmokeTestSupport::Stub stub(channel);
            ::grpc::ClientContext context;

            smoke_test::proto::DisposeSessionRequest request{};
            request.set_session_id(session_id_);
            smoke_test::proto::DisposeSessionResponse response{};
            ::grpc::Status status = stub.DisposeSession(&context, request, &response);
            if (status.error_code() !=  ::grpc::StatusCode::OK) {
                throw std::runtime_error(status.error_message ());
            }
        }
    }

private:
    std::string server_address_;
    std::uint64_t session_id_{};
};

}  // namespace
