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

#include <list>

#include <data_relay_grpc/common/session.h>

#include "session_impl.h"

namespace data_relay_grpc::common {

blob_session::blob_session(std::unique_ptr<blob_session_impl> impl) : impl_(std::move(impl)) {
}

blob_session::session_id_type blob_session::session_id() const noexcept {
    return impl_->session_id();
}

void blob_session::dispose() {
    impl_->dispose();
}

blob_session::blob_id_type blob_session::add(blob_path_type path) {
    return impl_->add(path);
}

std::optional<blob_session::blob_path_type> blob_session::find(blob_id_type blob_id) const {
    return impl_->find(blob_id);
}

std::vector<blob_session::blob_id_type> blob_session::entries() const {
    return impl_->entries();
}

template<class Iter>
void blob_session::remove(Iter, Iter) {
    throw std::runtime_error("not supported");
}

using vector_iterator = std::vector<blob_session::blob_id_type>::iterator;
template <>
void blob_session::remove<vector_iterator>(vector_iterator blob_ids_begin, vector_iterator blob_ids_end) {
    if (std::distance(blob_ids_begin, blob_ids_end) >= 0) {
        for (auto i = blob_ids_begin; i != blob_ids_end; i++) {
            impl_->delete_blob_file(*i);
        }
        return;
    }
    throw std::runtime_error("iterator begin is greather than iterator end");
}

using list_iterator = std::list<blob_session::blob_id_type>::iterator;
template <>
void blob_session::remove<list_iterator>(list_iterator blob_ids_begin, list_iterator blob_ids_end) {
    for (auto i = blob_ids_begin; i != blob_ids_end; i++) {
        impl_->delete_blob_file(*i);
    }
}

blob_session::blob_tag_type blob_session::compute_tag(blob_id_type blob_id) const {
    return impl_->compute_tag(blob_id);
}

} // namespace
