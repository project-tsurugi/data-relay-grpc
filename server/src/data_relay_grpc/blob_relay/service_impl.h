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

#include <tateyama/framework/component_ids.h>

#include <data_relay_grpc/blob_relay/service.h>

namespace data_relay_grpc::blob_relay {

class blob_session_manager;
class streaming_service;
class local_service;

/**
 * @brief blob relay service
 */
class blob_relay_service_impl : public blob_relay_service {
public:
    static constexpr id_type tag = tateyama::framework::resource_id_blob_relay_service;

    //@brief human readable label of this component
    static constexpr std::string_view component_label = "blob_relay_resource";

    [[nodiscard]] id_type id() const noexcept override;

    [[nodiscard]] std::string_view label() const noexcept override;

    bool setup(tateyama::framework::environment&) override;

    bool start(tateyama::framework::environment&) override;

    bool shutdown(tateyama::framework::environment&) override;

    blob_relay_service_impl(api const& f, service_configuration const& p);

    [[nodiscard]] blob_session& create_session(std::optional<std::uint64_t> transaction_id = std::nullopt) override;

    void add_blob_relay_service(::grpc::ServerBuilder& builder) override;

// for tests only
    blob_session_manager& get_session_manager();

private:
    api api_;
    service_configuration configuration_;

    using unique_ptr_session_manager = std::unique_ptr<blob_session_manager, void(*)(blob_session_manager*)>;
    using unique_ptr_streaming_service = std::unique_ptr<streaming_service, void(*)(streaming_service*)>;
    using unique_ptr_local_service = std::unique_ptr<local_service, void(*)(local_service*)>;
    unique_ptr_session_manager session_manager_;
    unique_ptr_streaming_service streaming_service_;
    unique_ptr_local_service local_service_;
};

} // namespace
