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

#include <data-relay-grpc/blob_relay/services.h>

#include "session_manager.h"
#include "streaming_service.h"
#include "local_service.h"

namespace data_relay_grpc::blob_relay {

services::services(api const& f, service_configuration const& c)
    : api_(f),
      configuration_(c),
      session_manager_(unique_ptr_session_manager(new blob_session_manager(api_, configuration_.session_store(), c.session_quota_size()), [](blob_session_manager* e){ delete e; })),
      streaming_service_(unique_ptr_streaming_service(new streaming_service(*session_manager_, configuration_.stream_chunk_size()), [](streaming_service* e){ delete  e; })),
      local_service_(unique_ptr_local_service(new local_service(*session_manager_), [](local_service* e){ delete  e; })) {
}

blob_session& services::create_session(std::optional<blob_session::transaction_id_type> transaction_id_opt) {
    return session_manager_->create_session(transaction_id_opt);
}

void services::add_blob_relay_services(grpc::ServerBuilder& builder) {
    builder.RegisterService(streaming_service_.get());
    if (configuration_.local_enabled()) {
        builder.RegisterService(local_service_.get());
    }
}

blob_session_manager& services::get_session_manager() {
    return *session_manager_;
}

} // namespace
