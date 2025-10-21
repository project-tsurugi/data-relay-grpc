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

#include <memory>
#include <cstdint>
#include <filesystem>

namespace data_relay_grpc::blob_relay {

/**
 * @brief blob relay service configuration
 */
class service_configuration {
public:
    service_configuration(
        bool enabled,
        const std::filesystem::path& session_store,
        std::size_t session_quota_size,
        bool local_enabled,
        bool local_upload_copy_file,
        std::size_t stream_chunk_size)
        : enabled_(enabled),
          session_store_(session_store), 
          session_quota_size_(session_quota_size),
          local_enabled_(local_enabled),
          local_upload_copy_file_(local_upload_copy_file),
          stream_chunk_size_(stream_chunk_size) {
    }

    bool enabled() {
        return enabled_;
    }
    std::filesystem::path session_store() const {
        return session_store_;
    }
    std::size_t session_quota_size() const {
        return session_quota_size_;
    }
    bool local_enabled() {
        return local_enabled_;
    }
    bool local_upload_copy_file() const {
        return local_upload_copy_file_;
    }
    std::size_t stream_chunk_size() const {
        return stream_chunk_size_;
    }

private:
    bool enabled_;
    std::filesystem::path session_store_;
    std::size_t session_quota_size_;
    bool local_enabled_;
    bool local_upload_copy_file_;
    std::size_t stream_chunk_size_;
};

} // namespace
