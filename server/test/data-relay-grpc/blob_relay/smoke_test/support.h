/*
 * Copyright 2025-2025 Project Tsurugi.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <mutex>

#include "data-relay-grpc/blob_relay/session_manager.h"

#include "blob_relay_smoke_test.pb.h"
#include "blob_relay_smoke_test.grpc.pb.h"

namespace data_relay_grpc::blob_relay::smoke_test {

class smoketest_support_service final : public proto::BlobRelaySmokeTestSupport::Service {
public:
    smoketest_support_service(blob_session_manager& session_manager) : session_manager_(session_manager) {
    }
    ~smoketest_support_service() override = default;

    smoketest_support_service(const smoketest_support_service&) = delete;
    smoketest_support_service& operator=(const smoketest_support_service&) = delete;
    smoketest_support_service(smoketest_support_service&&) = delete;
    smoketest_support_service& operator=(smoketest_support_service&&) = delete;

    ::grpc::Status CreateSession([[maybe_unused]] ::grpc::ServerContext* context,
                                 const proto::CreateSessionRequest* request,
                                 proto::CreateSessionResponse* response) {
        std::optional<std::uint64_t> transaxtion_id_opt{std::nullopt};
        if (request->context_id_case() == proto::CreateSessionRequest::ContextIdCase::kTransactionId) {
            transaxtion_id_opt = request->transaction_id();
        }
        auto& session = session_manager_.create_session(transaxtion_id_opt);
        response->set_session_id(session.session_id());
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    }

    ::grpc::Status DisposeSession([[maybe_unused]] ::grpc::ServerContext* context,
                                  const proto::DisposeSessionRequest* request,
                                  [[maybe_unused]] proto::DisposeSessionResponse* response) {
        auto session_id = request->session_id();
        session_manager_.get_session(session_id).dispose();
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    }

    ::grpc::Status GetFilePath([[maybe_unused]] ::grpc::ServerContext* context,
                               const proto::GetFilePathRequest* request,
                               proto::GetFilePathResponse* response) {
        auto session_id = request->session_id();
        auto blob_id = request->blob_id();
        try {
            auto& session = session_manager_.get_session(session_id);
            auto path_opt = session.find(blob_id);
            if (path_opt) {
                response->set_path(path_opt.value().string());
                return ::grpc::Status(::grpc::StatusCode::OK, "");
            }
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "can not find the blob file");
        } catch (std::runtime_error &ex) {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
        }
    }

    ::grpc::Status CreateBlobForDownLoad([[maybe_unused]] ::grpc::ServerContext* context,
                                         const proto::CreateBlobForDownLoadRequest* request,
                                         proto::CreateBlobForDownLoadResponse* response) {
        auto session_id = request->session_id();
        auto requested_size = static_cast<std::streamsize>(request->size());
        try {
            std::filesystem::path path("/tmp");
            path /= std::filesystem::path(std::to_string(getpid()) + "-" + std::to_string(++file_id_));
            std::ofstream blob_file(path);
            if (!blob_file) {
                return ::grpc::Status(::grpc::StatusCode::INTERNAL, "can not create a blob file for download test");
            }
            while (blob_file.tellp() < requested_size) {
                blob_file.write(ref_.data(), std::min(requested_size - blob_file.tellp(), static_cast<std::streamsize>(ref_.size())));
            }
            blob_file.close();

            auto& session = session_manager_.get_session(session_id);
            auto bid = session.add(path);

            auto* blob_reference = response->mutable_blob_reference();
            blob_reference->set_object_id(bid);
            blob_reference->set_tag(session.compute_tag(bid));
            return ::grpc::Status(::grpc::StatusCode::OK, "");
        } catch (std::runtime_error &ex) {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, ex.what());
        }
    }

private:
    blob_session_manager& session_manager_;
    std::string ref_{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n"};
    std::atomic_uint64_t file_id_{};
};

} // namespace data_relay_grpc::blob_relay::smoke_test
