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

#include <array>

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/err.h>

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
    std::array<unsigned char, sizeof(blob_session::blob_id_type) + sizeof(std::uint64_t)> input_bytes{};
    std::memcpy(input_bytes.data(), &blob_id, sizeof(blob_session::blob_id_type));
    std::memcpy(input_bytes.data() + sizeof(blob_session::blob_id_type), &session_id_, sizeof(std::uint64_t));

    auto const& secret_key = manager_.get_hmac_secret_key();

    ERR_clear_error();

    std::array<unsigned char, EVP_MAX_MD_SIZE> md{};
    unsigned int md_len = 0;

    unsigned char* result = HMAC(EVP_sha256(),
            secret_key.data(),
            static_cast<int>(secret_key.size()),
            input_bytes.data(),
            input_bytes.size(),
            md.data(),
            &md_len);

    if (! result) {
        std::string msg = "Failed to calculate reference tag: ";
        // NOLINTNEXTLINE(google-runtime-int) : OpenSSL API requires unsigned long
        unsigned long openssl_err = 0;
        bool has_error = false;
        while ((openssl_err = ERR_get_error()) != 0) {
            has_error = true;
            std::array<char, 256> err_msg_buf{};
            ERR_error_string_n(openssl_err,
                    err_msg_buf.data(),
                    err_msg_buf.size());
            msg += "[" + std::to_string(openssl_err) + ": "
                    + err_msg_buf.data() + "] ";
        }
        if (! has_error) {
            msg += "No OpenSSL error code available.";
        }
        throw std::runtime_error(msg);
    }

    blob_session::blob_tag_type tag = 0;
    std::memcpy(&tag, md.data(), sizeof(blob_session::blob_tag_type));

    return tag;
}

blob_session::blob_tag_type blob_session_impl::get_tag(blob_session::blob_id_type blob_id) const {
    if (transaction_id_opt_) {
        return manager_.get_tag(blob_id, transaction_id_opt_.value());
    }
    return manager_.get_tag(blob_id, session_id_);
}

} // namespace
