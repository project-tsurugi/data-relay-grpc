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

#include <mutex>

#include "data-relay-grpc/blob_relay/session_manager.h"

#include "blob_relay_smoke_test.pb.h"
#include "blob_relay_smoke_test.grpc.pb.h"

namespace data_relay_grpc::blob_relay::smoke_test {

using data_relay_grpc::blob_relay::smoke_test::proto::BlobRelaySmokeTestSupport;
using data_relay_grpc::blob_relay::smoke_test::proto::CreateSessionRequest;
using data_relay_grpc::blob_relay::smoke_test::proto::CreateSessionResponse;
using data_relay_grpc::blob_relay::smoke_test::proto::DisposeSessionRequest;
using data_relay_grpc::blob_relay::smoke_test::proto::DisposeSessionResponse;

class smoketest_support_service final : public BlobRelaySmokeTestSupport::Service {
public:
    smoketest_support_service(blob_session_manager& session_manager) : session_manager_(session_manager) {
    }
    ~smoketest_support_service() override = default;

    smoketest_support_service(const smoketest_support_service&) = delete;
    smoketest_support_service& operator=(const smoketest_support_service&) = delete;
    smoketest_support_service(smoketest_support_service&&) = delete;
    smoketest_support_service& operator=(smoketest_support_service&&) = delete;

    ::grpc::Status CreateSession([[maybe_unused]] ::grpc::ServerContext* context,
                                 [[maybe_unused]] const CreateSessionRequest* request,
                                 [[maybe_unused]] CreateSessionResponse* response) {
        auto& session = session_manager_.create_session(std::nullopt);
        response->set_session_id(session.session_id());
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    }

    ::grpc::Status DisposeSession([[maybe_unused]] ::grpc::ServerContext* context,
                                  const DisposeSessionRequest* request,
                                  [[maybe_unused]] DisposeSessionResponse* response) {
        auto session_id = request->session_id();
        session_manager_.dispose(session_id);
        return ::grpc::Status(::grpc::StatusCode::OK, "");
    }

private:
    blob_session_manager& session_manager_;
};

} // namespace data_relay_grpc::blob_relay::smoke_test
