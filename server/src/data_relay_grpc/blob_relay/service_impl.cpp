/*
 * Copyright 2025-2026 Project Tsurugi.
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

#include "service_impl.h"

#ifdef SMOKE_TEST_SUPPORT
#include "data_relay_grpc/blob_relay/smoke_test/support.h"
#endif

namespace data_relay_grpc::blob_relay {

#ifdef SMOKE_TEST_SUPPORT
namespace smoke_test {
static std::unique_ptr<smoketest_support_service> unqp_smoketest_support_service{};
}
#endif

blob_relay_service_impl::blob_relay_service_impl(common::api const& api, service_configuration const& conf)
    : api_(api),
      configuration_(conf),
      session_manager_(api_, configuration_.session_store(), configuration_.session_quota_size(), configuration_.dev_accept_mock_tag()),
      streaming_service_(std::make_unique<streaming_service>(session_manager_, configuration_.stream_chunk_size())) {
#ifdef SMOKE_TEST_SUPPORT
    smoke_test::unqp_smoketest_support_service = std::make_unique<smoke_test::smoketest_support_service>(session_manager_);
#endif
    if (streaming_service_) {
        services_.emplace_back(streaming_service_.get());
    }
    if (configuration_.local_enabled()) {
        local_service_ = std::make_unique<local_service>(session_manager_);
        services_.emplace_back(local_service_.get());
    }
#ifdef SMOKE_TEST_SUPPORT
    services_.emplace_back(smoke_test::unqp_smoketest_support_service.get());
#endif
}

blob_session& blob_relay_service_impl::create_session(std::optional<blob_session::transaction_id_type> transaction_id_opt) {
    return session_manager_.create_session(transaction_id_opt);
}

std::vector<::grpc::Service *>& blob_relay_service_impl::services() noexcept {
    return services_;
}

// for tests only
common::blob_session_manager& blob_relay_service_impl::get_session_manager() {
    return session_manager_;
}

} // namespace
