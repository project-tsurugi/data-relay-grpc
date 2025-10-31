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

#include <data-relay-grpc/blob_relay/session.h>
#include "session_manager.h"

namespace data_relay_grpc::blob_relay {

/**
 * @brief blob session impl
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
    [[nodiscard]] blob_session::blob_id_type add(blob_session::blob_path_type path) {
        std::lock_guard<std::mutex> lock(mtx_);
        blob_id_type new_blob_id = ++blob_id_;
        blobs_.emplace(new_blob_id, std::make_pair<blob_path_type, std::size_t>(session_store_.add_blob_file(path), std::filesystem::file_size(path)));
        return new_blob_id;
    }

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
      * @brief computes a tag value for the given BLOB ID.
      * @param blob_id the BLOB ID to compute the tag for.
      * @return the computed tag value.
      */
    [[nodiscard]] blob_session::blob_tag_type compute_tag(blob_session::blob_id_type blob_id) const {
        (void) blob_id;
        return 0;
    }

// internal use
    std::pair<blob_id_type, std::filesystem::path> create_blob_file() {
        std::lock_guard<std::mutex> lock(mtx_);
        blob_id_type new_blob_id = ++blob_id_;
        auto file_path = session_store_.create_blob_file(session_id_, new_blob_id);  // blob_id is unique within a session
        blobs_.emplace(new_blob_id, std::make_pair<blob_path_type, std::size_t>(blob_path_type(file_path), 0));  // the actual file does not exist
        return { new_blob_id, file_path };
    }
    std::optional<transaction_id_type> get_transaction_id() {
        return transaction_id_opt_;
    }
    bool reserve_session_store(blob_id_type bid, std::size_t size) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (session_store_.reserve(size)) {
            blobs_.at(bid).second += size;
            return true;
        }
        return false;
    }

private:
    session_id_type session_id_;
    blob_session_store& session_store_;
    std::optional<transaction_id_type> transaction_id_opt_;
    blob_session_manager& manager_;

    bool valid_{};
    std::atomic<blob_id_type> blob_id_{};

    std::map<blob_id_type, std::pair<blob_path_type, std::size_t>> blobs_{};
    mutable std::mutex mtx_{};

    friend class blob_session;
    friend class blob_session_manager;
    friend class streaming_service;
    friend class local_service;
    friend class stream_quota_test; // for test

    void delete_blob_file(blob_id_type bid) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (auto itr = blobs_.find(bid); itr != blobs_.end()) {
            session_store_.remove(itr->second.second);         // decrease session storage usage counter
            if (std::filesystem::exists(itr->second.first)) {  // maybe blob file has been moved
                std::filesystem::remove(itr->second.first);
            }
            blobs_.erase(itr);
            return;
        }
    }
};

} // namespace
