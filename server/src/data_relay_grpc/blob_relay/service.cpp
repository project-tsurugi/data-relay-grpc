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

#include <data_relay_grpc/blob_relay/service.h>

#include "session_manager.h"
#include "streaming_service.h"
#include "local_service.h"
#ifdef SMOKE_TEST_SUPPORT
#include "data_relay_grpc/blob_relay/smoke_test/support.h"
#endif

namespace data_relay_grpc::blob_relay {

#ifdef SMOKE_TEST_SUPPORT
namespace smoke_test {
static std::unique_ptr<smoketest_support_service> unqp_smoketest_support_service{};
}
#endif

blob_relay_service::blob_relay_service(api const& f, service_configuration const& c)
    : api_(f),
      configuration_(c),
      session_manager_(unique_ptr_session_manager(new blob_session_manager(api_, configuration_.session_store(), c.session_quota_size(), c.dev_accept_mock_tag()), [](blob_session_manager* e){ delete e; })),
      streaming_service_(unique_ptr_streaming_service(new streaming_service(*session_manager_, configuration_.stream_chunk_size()), [](streaming_service* e){ delete  e; })),
      local_service_(unique_ptr_local_service(new local_service(*session_manager_), [](local_service* e){ delete  e; })) {
#ifdef SMOKE_TEST_SUPPORT
    smoke_test::unqp_smoketest_support_service = std::make_unique<smoke_test::smoketest_support_service>(*session_manager_);
#endif
}

blob_session& blob_relay_service::create_session(std::optional<blob_session::transaction_id_type> transaction_id_opt) {
    return session_manager_->create_session(transaction_id_opt);
}

void blob_relay_service::add_blob_relay_service(grpc::ServerBuilder& builder) {
    builder.RegisterService(streaming_service_.get());
    if (configuration_.local_enabled()) {
        builder.RegisterService(local_service_.get());
    }
#ifdef SMOKE_TEST_SUPPORT
    builder.RegisterService(smoke_test::unqp_smoketest_support_service.get());
#endif
}

blob_session_manager& blob_relay_service::get_session_manager() {
    return *session_manager_;
}

} // namespace
