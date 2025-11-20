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

blob_session_manager::blob_session_manager(const blob_relay_service::api& api, const std::string& directory, std::size_t quota)
    : api_(api), session_store_(directory, quota) {
}

blob_session& blob_session_manager::create_session(std::optional<blob_session::transaction_id_type> transaction_id_opt) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto session_id = ++session_id_;
    blob_sessions_.emplace(session_id, blob_session(std::make_unique<blob_session_impl>(session_id, session_store_, transaction_id_opt, *this)));
    if (transaction_id_opt) {
        blob_session_ids_.emplace(transaction_id_opt.value(), session_id);
    }
    return blob_sessions_.at(session_id);
}

blob_session& blob_session_manager::get_session(blob_session::session_id_type session_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (auto&& itrs = blob_sessions_.find(session_id); itrs != blob_sessions_.end()) {
        return itrs->second;
    }
    throw std::out_of_range("can not find the session specified");
}

void blob_session_manager::dispose(blob_session::session_id_type session_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (auto&& itrs = blob_sessions_.find(session_id); itrs != blob_sessions_.end()) {
        if(auto transaction_id_opt = itrs->second.impl_->transaction_id_opt_; transaction_id_opt) {
            if (auto&& itrt = blob_session_ids_.find(transaction_id_opt.value()); itrt != blob_session_ids_.end()) {
                blob_session_ids_.erase(itrt);
            }
        }
        blob_sessions_.erase(itrs);
    }
}

blob_session_impl& blob_session_manager::get_session_impl(blob_session::session_id_type session_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (auto itr = blob_sessions_.find(session_id); itr != blob_sessions_.end()) {
        return *(itr->second.impl_);
    }
    throw std::out_of_range("can not find the session specified");
}

blob_session::session_id_type blob_session_manager::get_session_id(blob_session::transaction_id_type transaction_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (auto itr = blob_session_ids_.find(transaction_id); itr != blob_session_ids_.end()) {
        return itr->second;
    }
    throw std::out_of_range("can not find the session specified by the transaction_id");
}

blob_session::blob_tag_type blob_session_manager::get_tag(blob_session::blob_id_type bid, blob_session::transaction_id_type tid) {
    return api_.get_tag()(bid, tid);
}

blob_session::blob_path_type blob_session_manager::get_path(blob_session::blob_id_type bid) {
    return api_.get_path()(bid);
}

blob_session::blob_id_type blob_session_manager::get_new_blob_id() {
    return blob_id_.fetch_add(1) + 1;
}

} // namespace
