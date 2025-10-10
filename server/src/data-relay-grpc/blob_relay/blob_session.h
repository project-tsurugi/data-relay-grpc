/*
 * Copyright 2024-2025 Project Tsurugi.
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

#include <cstdint>
#include <map>
#include <optional>
#include <filesystem>
#include <atomic>

namespace data_relay_grpc::blob_relay {

/**
 * @brief blob session
 */
class blob_session {
public:
    /// @brief the session ID type.
    using session_id_type = std::uint64_t;

    /// @brief the transaction ID type.
    using transaction_id_type = std::uint64_t;

    /// @brief the BLOB ID type.
    using blob_id_type = std::uint64_t;

    /// @brief the BLOB file path type.
    using blob_path_type = std::filesystem::path;

    /// @brief the BLOB tag type.
    using blob_tag_type = std::uint64_t;

    blob_session(session_id_type session_id, blob_path_type&& directory, std::optional<blob_session::transaction_id_type> transaction_id_opt)
        : session_id_(session_id), directory_(directory), transaction_id_opt_(transaction_id_opt) {}

    std::optional<transaction_id_type> get_transaction_id() {
        return transaction_id_opt_;
    }

    std::pair<blob_id_type, std::filesystem::path> create_blob_file() {
        blob_id_type new_blob_id = blob_id_++;
        std::filesystem::path file_path = directory_;
        file_path /= std::filesystem::path(std::to_string(session_id_) + "_blob_" + std::to_string(new_blob_id));
        return { new_blob_id, file_path };
    }

private:
    session_id_type session_id_;
    blob_path_type directory_;
    std::atomic<blob_id_type> blob_id_{};

    // for download
    std::optional<transaction_id_type> transaction_id_opt_{};
    blob_path_type download_{};

    // for upload
    std::map<blob_id_type, blob_path_type> uploads_{};
};

} // namespace
