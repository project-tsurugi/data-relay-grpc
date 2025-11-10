#pragma once

#include <cstdint>
#include <memory>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <gflags/gflags.h>

#include "tateyama/transport/transport.h"
#include "tateyama/proto/debug/request.pb.h"
#include "tateyama/proto/debug/response.pb.h"

DECLARE_bool(dispose);

namespace tateyama {

class session {
public:
    session() {
        while (true) {
            try {
                transport_ = std::make_unique<tateyama::bootstrap::wire::transport>(tateyama::bootstrap::wire::transport::service_id_debug);
                return;
            } catch (std::runtime_error &ex) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            } catch (std::exception &ex) {
                throw std::runtime_error(std::string("originally: ") + ex.what());
            }
        }
    }
    ~session() {
        dispose();
        transport_.reset();
    }
    std::size_t session_id() noexcept {
        try {
            ::tateyama::proto::debug::request::Request request{};
            (void) request.mutable_blob_relay_create_session();
            auto response_opt = transport_->send<::tateyama::proto::debug::response::BlobRelayCreateSession>(request);

            if (response_opt) {
                const auto& response = response_opt.value();
                switch(response.result_case()) {
                case ::tateyama::proto::debug::response::BlobRelayCreateSession::ResultCase::kSessionId:
                    session_id_ = response.session_id();
                    return session_id_;
                default:
                    throw std::runtime_error("server error");
                }
            }
        } catch (std::runtime_error &ex) {
            std::cerr << ex.what() << std::endl;
        }
        return -1;
    }
    void dispose() const noexcept {
        if (FLAGS_dispose) {
            try {
                ::tateyama::proto::debug::request::Request request{};
                (request.mutable_blob_relay_dispose_session())->set_session_id(session_id_);
                auto response_opt = transport_->send<::tateyama::proto::debug::response::BlobRelayDisposeSession>(request);

                if (response_opt) {
                    const auto& response = response_opt.value();
                    switch(response.result_case()) {
                    case ::tateyama::proto::debug::response::BlobRelayDisposeSession::ResultCase::kSuccess:
                        return;
                    default:
                        throw std::runtime_error("server error");
                    }
                }
            } catch (std::runtime_error &ex) {
                std::cerr << ex.what() << std::endl;
            }
        }
    }

private:
    std::unique_ptr<tateyama::bootstrap::wire::transport> transport_{};
    std::uint64_t session_id_{};
};

}  // namespace
