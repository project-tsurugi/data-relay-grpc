/*
 * Copyright 2025-2026 Project Tsurugi.
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

#include <optional>
#include <cstdint>

#include <data_relay_grpc/common/session.h>

namespace data_relay_grpc::common {

/**
 * @brief blob relay service base
 */
class service {
public:
    /**
      * @brief Create a new session for BLOB operations.
      * @param transaction_id The ID of the transaction that owns the session,
      *    or empty if the session is not associated with any transaction
      * @return the created session object
      */
    [[nodiscard]] virtual blob_session& create_session(std::optional<std::uint64_t> transaction_id = std::nullopt) = 0;

protected:
    static std::shared_ptr<blob_session_manager> session_manager_;
};

} // namespace
