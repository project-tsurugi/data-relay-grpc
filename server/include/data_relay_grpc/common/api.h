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

#include <functional>
#include <filesystem>

#include <data_relay_grpc/common/session.h>

namespace data_relay_grpc::common {

/**
 * @brief a class holding tag related functions in datastore
 */
class api {
public:
    api(const std::function<blob_session::blob_tag_type(blob_session::blob_id_type, blob_session::transaction_id_type)> get_tag,
        const std::function<std::filesystem::path(blob_session::blob_id_type)> get_path)
        : get_tag_(get_tag), get_path_(get_path) {}

    api(api const&) = default;
    api(api&&) = delete;
    api& operator=(api const&) = default;
    api& operator=(api&&) = delete;

    const std::function<blob_session::blob_tag_type(blob_session::blob_id_type, blob_session::transaction_id_type)>& get_tag() {return get_tag_; }
    const std::function<std::filesystem::path(blob_session::blob_id_type)>& get_path() { return get_path_; }

private:
    std::function<blob_session::blob_tag_type(blob_session::blob_id_type, blob_session::transaction_id_type)> get_tag_;
    std::function<std::filesystem::path(blob_session::blob_id_type)> get_path_;
};

} // namespace
