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
#include "session_impl.h"

namespace data_relay_grpc::blob_relay {

blob_session::blob_id_type blob_session_impl::add(blob_session::blob_path_type path) {
    std::lock_guard<std::mutex> lock(mtx_);
    blob_id_type new_blob_id = manager_.get_new_blob_id();
    if (std::filesystem::exists(path)) {
        // the blob file is not subject to quota management
        blobs_.emplace(new_blob_id, std::make_pair<blob_path_type, std::size_t>(std::filesystem::canonical(path), 0));
        return new_blob_id;
    }
    throw std::runtime_error(path.string() + " does not exists");
}

void blob_session_impl::dispose() {
    for (auto&& e: entries()) {
        delete_blob_file(e);
    }
    manager_.dispose(session_id_);
}

std::pair<blob_session::blob_id_type, std::filesystem::path> blob_session_impl::create_blob_file(const std::string prefix) {
    std::lock_guard<std::mutex> lock(mtx_);
    blob_id_type new_blob_id = manager_.get_new_blob_id();
    auto file_path = session_store_.create_blob_file(new_blob_id, prefix);
    blobs_.emplace(new_blob_id, std::make_pair<blob_path_type, std::size_t>(blob_path_type(file_path), 0));  // the actual file does not exist
    return { new_blob_id, file_path };
}

blob_session::blob_tag_type blob_session_impl::compute_tag(blob_session::blob_id_type blob_id) const {
    return manager_.generate_reference_tag(blob_id, session_id_);
}

blob_session::blob_tag_type blob_session_impl::get_tag(blob_session::blob_id_type blob_id) const {
    if (transaction_id_opt_) {
        return manager_.get_tag(blob_id, transaction_id_opt_.value());
    }
    return manager_.get_tag(blob_id, session_id_);
}

} // namespace
