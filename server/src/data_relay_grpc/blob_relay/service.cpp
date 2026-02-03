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

#include <data_relay_grpc/blob_relay/service.h>
#include "service_impl.h"

namespace data_relay_grpc::blob_relay {

blob_relay_service::blob_relay_service(api const& api, service_configuration const& conf)
    : impl_(std::unique_ptr<blob_relay_service_impl, void(*)(blob_relay_service_impl*)>(new blob_relay_service_impl(api, conf), [](blob_relay_service_impl* e){ delete  e; })) {
    if (!session_manager_) {
        session_manager_ = std::make_shared<common::detail::blob_session_manager>(api, conf.session_store(), conf.session_quota_size(), conf.dev_accept_mock_tag());
    }
    impl_->session_manager_ = session_manager_;
}

blob_session& blob_relay_service::create_session(std::optional<blob_session::transaction_id_type> transaction_id_opt) {
    return impl_->create_session(transaction_id_opt);
}

const std::vector<::grpc::Service *>& blob_relay_service::services() const noexcept {
    return impl_->services();
}

blob_relay_service_impl& blob_relay_service::impl() const noexcept {
    return *impl_;
}

} // namespace
