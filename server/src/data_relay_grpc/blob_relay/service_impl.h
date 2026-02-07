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
#pragma once

#include <vector>

#include <data_relay_grpc/blob_relay/service.h>
#include <data_relay_grpc/common/detail/session_manager.h>

#include "streaming_service.h"
#include "local_service.h"

namespace data_relay_grpc::blob_relay {

class streaming_service;
class local_service;

/**
 * @brief blob relay service
 */
class blob_relay_service_impl {
public:
    blob_relay_service_impl(common::api const& api, service_configuration const& conf);

    [[nodiscard]] blob_session& create_session(std::optional<std::uint64_t> transaction_id = std::nullopt);

    std::vector<::grpc::Service *>& services() noexcept;

    common::detail::blob_session_manager& get_session_manager();

private:
    common::api api_;
    service_configuration configuration_;
    common::detail::blob_session_manager session_manager_;
    std::unique_ptr<streaming_service> streaming_service_;

    std::unique_ptr<local_service> local_service_{};
    std::vector<::grpc::Service *> services_{};

    friend class blob_relay_service;
};

} // namespace
