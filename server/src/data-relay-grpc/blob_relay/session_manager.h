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

#include <data-relay-grpc/blob_relay/services.h>
#include "session.h"

namespace data_relay_grpc::blob_relay {

/**
 * @brief blob session
 */
class blob_session_manager {
public:
    blob_session_manager(services::api const&, std::string);

    void create_session(blob_session::session_id_type, std::optional<blob_session::transaction_id_type>);

    blob_session& get_session(blob_session::session_id_type);

    blob_session::blob_tag_type get_tag(blob_session::transaction_id_type, blob_session::blob_id_type);

    blob_session::blob_path_type get_path(blob_session::blob_id_type);
    
  private:
    services::api api_;
    std::filesystem::path directory_;

    std::map<blob_session::session_id_type, std::unique_ptr<blob_session>> blob_sessions_{};
    std::map<blob_session::transaction_id_type, blob_session::session_id_type> blob_session_ids{};
};

} // namespace
