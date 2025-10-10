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

#include <limestone/api/datastore.h>

#include "data-relay-grpc/blob_relay/blob_session.h"

namespace data_relay_grpc::blob_relay::limestone {

class api_helper {
public:
    api_helper(::limestone::api::datastore& datastore) : datastore_(datastore) {}

    /**
     * @brief destroys this object.
     */
    virtual ~api_helper() = default;

    api_helper(api_helper const&) = delete;
    api_helper(api_helper&&) = delete;
    api_helper& operator=(api_helper const&) = delete;
    api_helper& operator=(api_helper&&) = delete;

    blob_session::blob_tag_type get_tag(blob_session::transaction_id_type, blob_session::blob_id_type) {
        return 111; // FIXME get tag from limestone with transaction_id and blob_id
    }

    blob_session::blob_path_type get_path(blob_session::blob_tag_type tag) {
        return datastore_.get_blob_file(tag).path().c_str();
    }

private:
    ::limestone::api::datastore& datastore_;
};

} // namespace data_relay_grpc::blob_relay::limestone
