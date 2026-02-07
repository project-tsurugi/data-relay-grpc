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

#include <optional>
#include <functional>
#include <filesystem>
#include <memory>

#include <data_relay_grpc/common/session.h>
#include <data_relay_grpc/common/api.h>
#include <data_relay_grpc/common/service.h>
#include <data_relay_grpc/blob_relay/service_configuration.h>

namespace data_relay_grpc::blob_relay {

using data_relay_grpc::common::blob_session;

class blob_relay_service_impl;

/**
 * @brief blob relay service
 */
class blob_relay_service : public common::service {
public:
    using api = data_relay_grpc::common::api;

    /**
      * @brief Create an object.
      * @param api the api for tag related calculations
      * @param conf the service_configuration
      */
    blob_relay_service(blob_relay_service::api const& api, service_configuration const& conf);

    /**
     * @brief Destruct the object.
     */
    virtual ~blob_relay_service() = default;

    /**
      * @brief Create a new session for BLOB operations.
      * @param transaction_id The ID of the transaction that owns the session,
      *    or empty if the session is not associated with any transaction
      * @return the created session object
      */
    [[nodiscard]] blob_session& create_session(std::optional<std::uint64_t> transaction_id = std::nullopt);

    [[nodiscard]] const std::vector<::grpc::Service *>& services() const noexcept override;

    [[nodiscard]] common::blob_session_manager& get_session_manager() const noexcept override;

private:
    std::unique_ptr<blob_relay_service_impl, void(*)(blob_relay_service_impl*)> impl_;
};

} // namespace
