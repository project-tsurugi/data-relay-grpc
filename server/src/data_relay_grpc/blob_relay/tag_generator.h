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

#include <array>
#include <string>
#include <stdexcept>

#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>

namespace data_relay_grpc::blob_relay {

template <typename T1, typename T2, typename T3>
class tag_generator {
  public:
    tag_generator() {
        // The generated key is 128 bits (16 bytes). Although HMAC-SHA256 can use
        // longer keys (and RFC 2104 recommends keys at least as long as the hash
        // output), we keep the 16-byte key length for compatibility with existing
        // systems that assume this size.
        if (RAND_bytes(hmac_secret_key_.data(), static_cast<int>(hmac_secret_key_.size())) != 1) {
            throw std::runtime_error("Failed to generate random bytes for HMAC secret key for BLOB reference tag generation");
        }
    }
    T3 generate_reference_tag(
        T1 p1,
        T2 p2) {

        // Prepare input data: concatenate p1 and p2 using portable approach
        std::array<unsigned char, sizeof(T1) + sizeof(T2)> input_bytes{};
        std::memcpy(input_bytes.data(), &p1, sizeof(T1));
        std::memcpy(input_bytes.data() + sizeof(T1), &p2, sizeof(T2));

        // Clear OpenSSL error queue to avoid noise from previous API calls
        ERR_clear_error();

        // Calculate HMAC-SHA256
        std::array<unsigned char, EVP_MAX_MD_SIZE> md{};
        unsigned int md_len = 0;

        unsigned char* result = HMAC(EVP_sha256(),
                                     hmac_secret_key_.data(),
                                     static_cast<int>(hmac_secret_key_.size()),
                                     input_bytes.data(),
                                     input_bytes.size(),
                                     md.data(),
                                     &md_len);

        if (result == nullptr) {
            throw std::runtime_error(hmac_error_message());
        }

        // Use the first 8 bytes of the HMAC result as the tag
        T3 tag = 0;
        std::memcpy(&tag, md.data(), sizeof(T3));

        return tag;
    }

  private:
    // HMAC secret key for BLOB reference tag generation (128-bit, 16 bytes).
    // Note:
    //   - HMAC-SHA256 accepts keys of any length, but RFC 2104 recommends
    //     using a key at least as long as the hash output (256 bits).
    //   - This implementation intentionally uses a 128-bit random key because
    //     the key size is constrained by existing deployments (e.g. stored
    //     configuration / wire format) and changing it would invalidate
    //     previously generated BLOB reference tags.
    //   - A 128-bit uniformly random secret key still provides strong security
    //     for this use case, and the choice is documented here for clarity.
    std::array<std::uint8_t, 16> hmac_secret_key_{};

    std::string hmac_error_message() {
        // Retrieve all OpenSSL error codes and error strings
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
            msg += "[" + std::to_string(openssl_err) + ": " + err_msg_buf.data() + "] ";
        }
        if (! has_error) {
            msg += "No OpenSSL error code available.";
        }
        return msg;
    }
};

}
