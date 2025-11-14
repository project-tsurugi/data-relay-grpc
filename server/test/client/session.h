#pragma once

#include <cstdint>
#include <optional>
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
    session(const std::string& server_address, const std::optional<uint64_t> transaction_id_opt) : server_address_(server_address), transaction_id_opt_(transaction_id_opt) {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        smoke_test::proto::BlobRelaySmokeTestSupport::Stub stub(channel);
        ::grpc::ClientContext context;

        smoke_test::proto::CreateSessionRequest request{};
        if (transaction_id_opt_) {
            request.set_transaction_id(transaction_id_opt_.value());
        }
        smoke_test::proto::CreateSessionResponse response{};
        ::grpc::Status status = stub.CreateSession(&context, request, &response);
        if (status.error_code() !=  ::grpc::StatusCode::OK) {
            throw std::runtime_error(status.error_message());
        }
        session_id_ = response.session_id();
    }
    session(const std::string& server_address) : session(server_address, std::nullopt) {
    }
    ~session() {
        dispose();
    }
    std::size_t session_id() {
        return session_id_;
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
    std::filesystem::path file_path(std::uint64_t blob_id) {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        smoke_test::proto::BlobRelaySmokeTestSupport::Stub stub(channel);
        ::grpc::ClientContext context;

        smoke_test::proto::GetFilePathRequest request{};
        request.set_session_id(session_id_);
        request.set_blob_id(blob_id);
        smoke_test::proto::GetFilePathResponse response{};
        ::grpc::Status status = stub.GetFilePath(&context, request, &response);
        if (status.error_code() !=  ::grpc::StatusCode::OK) {
            throw std::runtime_error(status.error_message());
        }
        return { response.path() };
    }
    std::pair<std::uint64_t, std::uint64_t> create_blob_file_for_download(std::uint64_t size) {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        smoke_test::proto::BlobRelaySmokeTestSupport::Stub stub(channel);
        ::grpc::ClientContext context;

        smoke_test::proto::CreateBlobForDownLoadRequest request{};
        request.set_session_id(session_id_);
        if (transaction_id_opt_) {
            request.set_transaction_id(transaction_id_opt_.value());
        }
        request.set_size(size);
        smoke_test::proto::CreateBlobForDownLoadResponse response{};
        ::grpc::Status status = stub.CreateBlobForDownLoad(&context, request, &response);
        if (status.error_code() !=  ::grpc::StatusCode::OK) {
            throw std::runtime_error(status.error_message());
        }
        auto& blob_reference = response.blob_reference();
        return { blob_reference.object_id(), blob_reference.tag() };
    }

private:
    std::string server_address_;
    std::uint64_t session_id_{};
    std::optional<std::uint64_t> transaction_id_opt_{};
};

}  // namespace
