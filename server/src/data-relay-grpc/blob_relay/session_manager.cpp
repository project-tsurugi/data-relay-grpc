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

#include "session_manager.h"

namespace data_relay_grpc::blob_relay {

blob_session_manager::blob_session_manager(services::api const& api, std::string directory)
    : api_(api), directory_(directory) {
}

void blob_session_manager::create_session(blob_session::session_id_type session_id, std::optional<blob_session::transaction_id_type> transaction_id_opt) {
    blob_sessions_.emplace(session_id, std::make_unique<blob_session>(session_id, directory_, transaction_id_opt));
}

blob_session& blob_session_manager::get_session(blob_session::session_id_type session_id) {
    return *blob_sessions_.at(session_id);
}

blob_session::blob_tag_type blob_session_manager::get_tag(blob_session::transaction_id_type tid, blob_session::blob_id_type bid) {
    return api_.get_tag()(tid, bid);
}

blob_session::blob_path_type blob_session_manager::get_path(blob_session::blob_id_type bid) {
    return api_.get_path()(bid);
}

} // namespace
