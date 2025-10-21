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

#include <optional>
#include <functional>
#include <filesystem>

#include <grpcpp/grpcpp.h>

#include <data-relay-grpc/blob_relay/service_configuration.h>

namespace data_relay_grpc::blob_relay {

class blob_session_manager;
class streaming_service;
class local_service;

/**
 * @brief blob relay services
 */
class services {
public:
    class api {
    public:
        api(const std::function<std::uint64_t(std::uint64_t, std::uint64_t)> get_tag,
            const std::function<std::filesystem::path(std::uint64_t)> get_path)
            : get_tag_(get_tag), get_path_(get_path) {}

        api(api const&) = default;
        api(api&&) = delete;
        api& operator=(api const&) = default;
        api& operator=(api&&) = delete;

        const std::function<std::uint64_t(std::uint64_t, std::uint64_t)>& get_tag() {return get_tag_; }
        const std::function<std::filesystem::path(std::uint64_t)>& get_path() { return get_path_; }

    private:
        std::function<std::uint64_t(std::uint64_t, std::uint64_t)> get_tag_;
        std::function<std::filesystem::path(std::uint64_t)> get_path_;
    };

    services(api const& f, service_configuration const& p);

    void create_session(std::uint64_t, std::optional<std::uint64_t>);
    void add_blob_relay_services(::grpc::ServerBuilder&);

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
