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

#include <grpcpp/grpcpp.h>

namespace data_relay_grpc::blob_relay {

/**
 * @brief blob relay services
 */
class services {
public:
    services() {
    }

    void add_blob_relay_streaming_service(grpc::ServerBuilder&) {}  // FIXME
    void add_blob_relay_local_service(grpc::ServerBuilder&) {}  // FIXME
};

} // namespace
