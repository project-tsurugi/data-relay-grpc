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
#include <mutex>

#include <data_relay_grpc/blob_relay/service.h>
#include "session_store.h"
#include "session_impl.h"

namespace data_relay_grpc::blob_relay {

/**
 * @brief blob session
 */
class blob_session_manager {
public:
    blob_session_manager(const blob_relay_service::api&, const std::string&, std::size_t, bool);

    blob_session& create_session(std::optional<blob_session::transaction_id_type>);

    void dispose(blob_session::session_id_type);

    blob_session& get_session(blob_session::session_id_type);

    blob_session::blob_tag_type get_tag(blob_session::blob_id_type, blob_session::transaction_id_type);

    blob_session::blob_path_type get_path(blob_session::blob_id_type);

    blob_session_impl& get_session_impl(blob_session::session_id_type);

    blob_session::session_id_type get_session_id(blob_session::transaction_id_type);

    bool dev_accept_mock_tag() {
        return dev_accept_mock_tag_;
    }
    constexpr static blob_session::blob_tag_type MOCK_TAG = 0xffffffffffffffffLL;

private:
    blob_relay_service::api api_;
    blob_session_store session_store_;
    bool dev_accept_mock_tag_;
    std::atomic<blob_session::session_id_type> session_id_{};
    std::atomic<blob_session::blob_id_type> blob_id_{};

    std::map<blob_session::session_id_type, blob_session> blob_sessions_{};
    std::map<blob_session::transaction_id_type, blob_session::session_id_type> blob_session_ids_{};
    mutable std::mutex mtx_{};

    friend class blob_session_impl;
    blob_session::blob_id_type get_new_blob_id();
};

} // namespace
