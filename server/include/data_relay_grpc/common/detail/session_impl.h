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
#include <vector>
#include <atomic>
#include <mutex>

#include <data_relay_grpc/common/session.h>
#include <data_relay_grpc/common/detail/session_manager.h>

namespace data_relay_grpc::common::detail {

class blob_session_manager;

/**
 * @brief blob session impl class
 */
class blob_session_impl {
public:
    using session_id_type = blob_session::session_id_type;
    using transaction_id_type = blob_session::transaction_id_type;
    using blob_id_type = blob_session::blob_id_type;
    using blob_path_type = blob_session::blob_path_type;
    using blob_tag_type = blob_session::blob_tag_type;

    blob_session_impl(session_id_type session_id, blob_session_store& session_store, std::optional<blob_session::transaction_id_type> transaction_id_opt, blob_session_manager& manager)
        : session_id_(session_id), session_store_(session_store), transaction_id_opt_(transaction_id_opt), manager_(manager) {}

    /**
     * @brief returns the ID of this session.
     * @return the session ID
     */
    [[nodiscard]] blob_session::session_id_type session_id() const noexcept {
        return session_id_;
    }

    /**
     * @brief dispose this session and release all resources associated with it.
     * @note After calling this method, the session becomes invalid and cannot be used anymore.
     * @attention please ensure to call this method to avoid resource leaks before this object is destroyed.
     */
    void dispose();

    /**
     * @brief adds a BLOB data file path to this session.
     * @param path the path to the BLOB data file to add.
     * @return the BLOB ID assigned to the added BLOB data file.
     * @attention undefined behavior occurs if the path is already added in this session.
     */
    [[nodiscard]] blob_session::blob_id_type add(blob_session::blob_path_type);

    /**
     * @brief find the BLOB data file path associated with the given BLOB ID.
     * @param blob_id the BLOB ID to retrieve
     * @return the path to the BLOB data file if found
     * @return otherwise, std::nullopt.
     */
    [[nodiscard]] std::optional<blob_session::blob_path_type> find(blob_session::blob_id_type blob_id) const {
        std::lock_guard<std::mutex> lock(mtx_);
        if (auto&& itr = blobs_.find(blob_id); itr != blobs_.end()) {
            return itr->second.first;
        }
        return std::nullopt;
    }

    /**
     * @brief returns a list of added BLOB IDs in this session.
     * @return the list of added BLOB IDs.
     */
    [[nodiscard]] std::vector<blob_session::blob_id_type> entries() const {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<blob_session::blob_id_type> v{};
        for (auto&& e: blobs_) {
            v.emplace_back(e.first);
        }
        return v;
    }

    /**
      * @brief computes a tag value for the given BLOB ID used in Put operation.
      * @param blob_id the BLOB ID to compute the tag for.
      * @return the computed tag value.
      */
    [[nodiscard]] blob_session::blob_tag_type compute_tag(blob_session::blob_id_type blob_id) const;

    /**
      * @brief generate a tag value for the given BLOB ID using api.get_tag.
      * @param blob_id the BLOB ID to compute the tag for.
      * @return the computed tag value.
      */
    [[nodiscard]] blob_session::blob_tag_type get_tag(blob_session::blob_id_type blob_id) const;

// below this point is for internal use
    std::pair<blob_id_type, std::filesystem::path> create_blob_file(const std::string prefix = "upload");

    void delete_blob_file(blob_id_type bid);

    [[nodiscard]] std::optional<transaction_id_type> get_transaction_id() const noexcept;

    bool reserve_session_store(blob_id_type bid, std::size_t size);

private:
    session_id_type session_id_;
    blob_session_store& session_store_;
    std::optional<transaction_id_type> transaction_id_opt_;
    blob_session_manager& manager_;

    bool valid_{};
    std::map<blob_id_type, std::pair<blob_path_type, std::size_t>> blobs_{};
    mutable std::mutex mtx_{};

    friend class blob_session;
    friend class blob_session_manager;
};

} // namespace
