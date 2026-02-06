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

#include <filesystem>
#include <atomic>

namespace data_relay_grpc::common::detail {

/**
 * @brief blob session
 */
class blob_session_store {
public:
    blob_session_store(const std::string& directory, std::size_t quota) : directory_(directory), quota_(quota) {
        namespace fs = std::filesystem;
    
        if (!fs::exists(directory_)) {
            throw std::runtime_error(directory_.string() + " does not exists");
        }
        fs::file_status status = fs::status(directory_);
        if (status.type() != fs::file_type::directory &&
            !(status.type() == fs::file_type::symlink && fs::symlink_status(directory).type() == fs::file_type::directory)) {
            throw std::runtime_error(directory_.string() + " is not a directory");
        }
        fs::perms perm = status.permissions();
        if ((perm & (fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write)) == fs::perms::none) {
            throw std::runtime_error(directory_.string() + " is not writable");
        }

        // remove existing files in the initialization
        for (const fs::directory_entry& itr : fs::directory_iterator(directory)) {
            std::error_code ec;
            fs::remove(itr.path(), ec);
            if (ec) {
                throw std::runtime_error(itr.path().string() + "remains in the session store directory (" + directory_.string() + ")");
            }
        }
    }

    // for test only
    std::size_t current_size() const noexcept {
        return current_size_.load();
    }

  private:
    std::filesystem::path directory_;
    std::size_t quota_;
    
    std::atomic<std::size_t> current_size_{};

    friend class blob_session_impl;
    friend class blob_session_manager;
    std::filesystem::path create_blob_file(std::uint64_t new_blob_id, const std::string& prefix) {
        std::filesystem::path file_path = directory_;
        return directory_ / std::filesystem::path(prefix + "_" + std::to_string(new_blob_id));
    }
    bool reserve(std::size_t size) {
        if (quota_ != 0) {
            std::size_t current = current_size_.load();
            while (true) {
                if (current + size > quota_) {
                    return false;
                }
                if (current_size_.compare_exchange_strong(current, current + size)) {
                    return true;
                }
            }
        }
        return true;
    }
    void remove(std::size_t size) {
        if (quota_ != 0) {
            current_size_.fetch_sub(size);
        }
    }
};

} // namespace
