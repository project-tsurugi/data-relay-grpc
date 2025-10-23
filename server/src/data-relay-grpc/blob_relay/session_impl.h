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

    blob_session_impl(session_id_type session_id, blob_path_type& directory, std::optional<blob_session::transaction_id_type> transaction_id_opt, blob_session_manager& manager)
        : session_id_(session_id), directory_(directory), transaction_id_opt_(transaction_id_opt), manager_(manager) {}

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
        blob_id_type new_blob_id = ++blob_id_;
        std::filesystem::path file_path = directory_;
        file_path /= path;
        blobs_.emplace(new_blob_id, file_path);
        return new_blob_id;
    }

    /**
     * @brief find the BLOB data file path associated with the given BLOB ID.
     * @param blob_id the BLOB ID to retrieve
     * @return the path to the BLOB data file if found
     * @return otherwise, std::nullopt.
     */
    [[nodiscard]] std::optional<blob_session::blob_path_type> find(blob_session::blob_id_type blob_id) const {
        if (auto&& itr = blobs_.find(blob_id); itr != blobs_.end()) {
            return itr->second;
        }
        return std::nullopt;
    }

    /**
     * @brief returns a list of added BLOB IDs in this session.
     * @return the list of added BLOB IDs.
     */
    [[nodiscard]] std::vector<blob_session::blob_id_type> entries() const {
        std::vector<blob_session::blob_id_type> v{};
        for (auto&& e: blobs_) {
            v.emplace_back(e.first);
        }
        return v;
    }

    /**
     * @brief removes the BLOB data file associated with the given BLOB IDs from this session.
     * @details if there is no BLOB data file associated with the given BLOB ID, this method does nothing.
     * @param blob_ids_begin the beginning of the range of BLOB IDs to remove.
     * @param blob_ids_end the end of the range of BLOB IDs to remove.
     */

// FIXME implement
//    template<class Iter>
//    void remove(Iter blob_ids_begin, Iter blob_ids_end);

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
        blob_id_type new_blob_id = ++blob_id_;
        std::filesystem::path file_path = directory_;
        file_path /= std::filesystem::path(std::string("upload_") + std::to_string(new_blob_id));
        blobs_.emplace(new_blob_id, file_path);
        return { new_blob_id, file_path };
    }
    std::optional<transaction_id_type> get_transaction_id() {
        return transaction_id_opt_;
    }

private:
    session_id_type session_id_;
    blob_path_type& directory_;
    std::optional<transaction_id_type> transaction_id_opt_;
    blob_session_manager& manager_;

    bool valid_{};
    std::atomic<blob_id_type> blob_id_{};

    std::map<blob_id_type, blob_path_type> blobs_{};

    friend class blob_session_manager;
};

} // namespace
