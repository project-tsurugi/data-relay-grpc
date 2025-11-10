/*
 * Copyright 2022-2025 Project Tsurugi.
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

#include <sstream>
#include <optional>
#include <exception>
#include <memory>
#include <sys/types.h>
#include <unistd.h>

#include <gflags/gflags.h>

#include <tateyama/utils/protobuf_utils.h>
#include <tateyama/proto/framework/request.pb.h>
#include <tateyama/proto/framework/response.pb.h>
#include <tateyama/proto/core/request.pb.h>
#include <tateyama/proto/core/response.pb.h>
#include <tateyama/proto/datastore/common.pb.h>
#include <tateyama/proto/datastore/request.pb.h>
#include <tateyama/proto/datastore/response.pb.h>
#include <tateyama/proto/endpoint/request.pb.h>
#include <tateyama/proto/endpoint/response.pb.h>
#include <tateyama/proto/session/request.pb.h>
#include <tateyama/proto/session/response.pb.h>
#include <tateyama/proto/metrics/request.pb.h>
#include <tateyama/proto/metrics/response.pb.h>
#include <tateyama/proto/debug/request.pb.h>
#include <tateyama/proto/debug/response.pb.h>
#include <tateyama/proto/diagnostics.pb.h>
#include <tateyama/proto/request/request.pb.h>
#include <tateyama/proto/request/response.pb.h>

#include "client_wire.h"
#include "timer.h"

DECLARE_string(dbname);  // NOLINT

namespace tateyama::bootstrap::wire {

constexpr static std::size_t HEADER_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t HEADER_MESSAGE_VERSION_MINOR = 1;
constexpr static std::size_t CORE_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t CORE_MESSAGE_VERSION_MINOR = 0;
constexpr static std::size_t DATASTORE_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t DATASTORE_MESSAGE_VERSION_MINOR = 0;
constexpr static std::size_t ENDPOINT_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t ENDPOINT_MESSAGE_VERSION_MINOR = 0;
constexpr static std::size_t SESSION_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t SESSION_MESSAGE_VERSION_MINOR = 0;
constexpr static std::size_t METRICS_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t METRICS_MESSAGE_VERSION_MINOR = 0;
#ifdef ENABLE_ALTIMETER
constexpr static std::size_t ALTIMETER_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t ALTIMETER_MESSAGE_VERSION_MINOR = 0;
#endif
constexpr static std::size_t REQUEST_MESSAGE_VERSION_MAJOR = 0;
constexpr static std::size_t REQUEST_MESSAGE_VERSION_MINOR = 0;
constexpr static std::size_t SQL_MESSAGE_VERSION_MAJOR = 1;
constexpr static std::size_t SQL_MESSAGE_VERSION_MINOR = 6;
constexpr static std::int64_t EXPIRATION_SECONDS = 60;

class transport {
public:
    transport() = delete;

    explicit transport(std::uint32_t type) :
        wire_(tateyama::common::wire::session_wire_container(tateyama::common::wire::connection_container(database_name()).connect())) {

        header_.set_service_message_version_major(HEADER_MESSAGE_VERSION_MAJOR);
        header_.set_service_message_version_minor(HEADER_MESSAGE_VERSION_MINOR);
        header_.set_service_id(type);

        try {
            auto handshake_response_opt = handshake();
            if (!handshake_response_opt) {
                throw std::runtime_error("handshake error");
            }
            auto& handshake_response = handshake_response_opt.value();
            if (handshake_response.result_case() != tateyama::proto::endpoint::response::Handshake::ResultCase::kSuccess) {
                auto& message = handshake_response.error().message();
                throw std::runtime_error(message.empty() ? "handshake error" : message);
            }
            session_id_ = handshake_response.success().session_id();
            header_.set_session_id(session_id_);

            timer_ = std::make_unique<tateyama::common::wire::timer>(EXPIRATION_SECONDS, [this](){
                auto ret = update_expiration_time();
                if (ret.has_value()) {
                    return ret.value().result_case() == tateyama::proto::core::response::UpdateExpirationTime::ResultCase::kSuccess;
                }
                return false;
            });
        } catch (std::runtime_error &ex) {
            close();
            throw ex;
        }
    }

    ~transport() {
        try {
            timer_ = nullptr;
            if (!closed_) {
                close();
            }
        } catch (std::exception &ex) {
            std::cerr << ex.what() << '\n' << std::flush;
        }
    }

    transport(transport const& other) = delete;
    transport& operator=(transport const& other) = delete;
    transport(transport&& other) noexcept = delete;
    transport& operator=(transport&& other) noexcept = delete;

    template <typename T>
    std::optional<T> send(::tateyama::proto::datastore::request::Request& request) {
        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(header_, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(DATASTORE_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(DATASTORE_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);        
        ::tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        T response{};
        if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        return response;
    }

    // for session
    template <typename T>
    std::optional<T> send(::tateyama::proto::session::request::Request& request) {
        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(header_, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(SESSION_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(SESSION_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);
        ::tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        if (header.payload_type() == tateyama::proto::framework::response::Header::SERVICE_RESULT) {
            T response{};
            if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
                return std::nullopt;
            }
            return response;
        }
        tateyama::proto::diagnostics::Record record{};
        if(auto res = record.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        throw_std_runtime_error(record);
        return std::nullopt;  // dummy to suppress compile error
    }

    // for debug
    template <typename T>
    std::optional<T> send(::tateyama::proto::debug::request::Request& request) {
        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(header_, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(SESSION_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(SESSION_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);
        ::tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        if (header.payload_type() == tateyama::proto::framework::response::Header::SERVICE_RESULT) {
            T response{};
            if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
                return std::nullopt;
            }
            return response;
        }
        tateyama::proto::diagnostics::Record record{};
        if(auto res = record.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        throw_std_runtime_error(record);
        return std::nullopt;  // dummy to suppress compile error
    }

    constexpr static std::uint32_t service_id_routing = 0;
    constexpr static std::uint32_t service_id_endpoint_broker = 1;
//    constexpr static std::uint32_t service_id_sql = 3;
    constexpr static std::uint32_t service_id_debug = 6;

// implement handshake
    template <typename T>
    std::optional<T> send(::tateyama::proto::endpoint::request::Request& request) {
        tateyama::proto::framework::request::Header fwrq_header{};
        fwrq_header.set_service_message_version_major(HEADER_MESSAGE_VERSION_MAJOR);
        fwrq_header.set_service_message_version_minor(HEADER_MESSAGE_VERSION_MINOR);
        fwrq_header.set_service_id(service_id_endpoint_broker);

        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(fwrq_header, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(ENDPOINT_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(ENDPOINT_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);
        tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        if (header.payload_type() == tateyama::proto::framework::response::Header::SERVICE_RESULT) {
            T response{};
            if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
                return std::nullopt;
            }
            return response;
        }
        tateyama::proto::diagnostics::Record record{};
        if(auto res = record.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        throw_std_runtime_error(record);
        return std::nullopt;  // dummy to suppress compile error
    }

    // expiration
    template <typename T>
    std::optional<T> send(::tateyama::proto::core::request::Request& request) {
        tateyama::proto::framework::request::Header fwrq_header{};
        fwrq_header.set_service_message_version_major(HEADER_MESSAGE_VERSION_MAJOR);
        fwrq_header.set_service_message_version_minor(HEADER_MESSAGE_VERSION_MINOR);
        fwrq_header.set_service_id(service_id_routing);

        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(fwrq_header, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(CORE_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(CORE_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);
        tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        if (header.payload_type() == tateyama::proto::framework::response::Header::SERVICE_RESULT) {
            T response{};
            if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
                return std::nullopt;
            }
            return response;
        }
        tateyama::proto::diagnostics::Record record{};
        if(auto res = record.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        throw_std_runtime_error(record);
        return std::nullopt;  // dummy to suppress compile error
    }

    // metrics
    template <typename T>
    std::optional<T> send(::tateyama::proto::metrics::request::Request& request) {
        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(header_, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(METRICS_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(METRICS_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);
        ::tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        if (header.payload_type() == tateyama::proto::framework::response::Header::SERVICE_RESULT) {
            T response{};
            if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
                return std::nullopt;
            }
            return response;
        }
        tateyama::proto::diagnostics::Record record{};
        if(auto res = record.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        throw_std_runtime_error(record);
        return std::nullopt;  // dummy to suppress compile error
    }

    // for request
    template <typename T>
    std::optional<T> send(::tateyama::proto::request::request::Request& request) {
        std::stringstream sst{};
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(header_, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        request.set_service_message_version_major(REQUEST_MESSAGE_VERSION_MAJOR);
        request.set_service_message_version_minor(REQUEST_MESSAGE_VERSION_MINOR);
        if(auto res = tateyama::utils::SerializeDelimitedToOstream(request, std::addressof(sst)); ! res) {
            return std::nullopt;
        }
        auto slot_index = wire_.search_slot();
        wire_.send(sst.str(), slot_index);

        std::string res_message{};
        wire_.receive(res_message, slot_index);
        ::tateyama::proto::framework::response::Header header{};
        google::protobuf::io::ArrayInputStream ins{res_message.data(), static_cast<int>(res_message.length())};
        if(auto res = tateyama::utils::ParseDelimitedFromZeroCopyStream(std::addressof(header), std::addressof(ins), nullptr); ! res) {
            return std::nullopt;
        }
        std::string_view payload{};
        if (auto res = tateyama::utils::GetDelimitedBodyFromZeroCopyStream(std::addressof(ins), nullptr, payload); ! res) {
            return std::nullopt;
        }
        if (header.payload_type() == tateyama::proto::framework::response::Header::SERVICE_RESULT) {
            T response{};
            if(auto res = response.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
                return std::nullopt;
            }
            return response;
        }
        tateyama::proto::diagnostics::Record record{};
        if(auto res = record.ParseFromArray(payload.data(), static_cast<int>(payload.length())); ! res) {
            return std::nullopt;
        }
        throw_std_runtime_error(record);
        return std::nullopt;  // dummy to suppress compile error
    }

    void close() {
        wire_.close();
        closed_ = true;
    }

    [[nodiscard]] std::size_t session_id() const noexcept {
        return session_id_;
    }
    [[nodiscard]] const std::string& encrypted_credential() const noexcept {
        return encrypted_credential_;
    }

    static std::string database_name() {
        return { FLAGS_dbname };
    }

private:
    tateyama::common::wire::session_wire_container wire_;
    tateyama::proto::framework::request::Header header_{};
    std::size_t session_id_{};
    bool closed_{};
    std::unique_ptr<tateyama::common::wire::timer> timer_{};
    std::string encrypted_credential_{};

    std::optional<tateyama::proto::endpoint::response::Handshake> handshake() {
        tateyama::proto::endpoint::request::Request request{};
        auto* handshake = request.mutable_handshake();
        auto* client_information = handshake->mutable_client_information();
        client_information->set_application_name("tateyama-test-client");
        auto* wire_information = handshake->mutable_wire_information();
        auto* ipc_information = wire_information->mutable_ipc_information();
        ipc_information->set_connection_information(std::to_string(getpid()));

        return send<tateyama::proto::endpoint::response::Handshake>(request);
    }

    // EncryptionKey
    std::optional<tateyama::proto::endpoint::response::EncryptionKey> encryption_key() {
        tateyama::proto::endpoint::request::Request request{};
        (void) request.mutable_encryption_key();

        return send<tateyama::proto::endpoint::response::EncryptionKey>(request);
    }

    // UpdateExpirationTime
    std::optional<tateyama::proto::core::response::UpdateExpirationTime> update_expiration_time() {
        tateyama::proto::core::request::UpdateExpirationTime uet_request{};

        tateyama::proto::core::request::Request request{};
        *(request.mutable_update_expiration_time()) = uet_request;

        return send<tateyama::proto::core::response::UpdateExpirationTime>(request);
    }

    // throw std::runtime_error
    void throw_std_runtime_error(const tateyama::proto::diagnostics::Record& record) const {
        if (record.code() == tateyama::proto::diagnostics::Code::PERMISSION_ERROR) {
            throw std::runtime_error(record.message());
        }
        throw std::runtime_error(record.message());
    }
};

} // tateyama::bootstrap::wire
