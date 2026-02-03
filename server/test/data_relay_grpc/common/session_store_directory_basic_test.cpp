#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <exception>

#include "test_root.h"
#include "data_relay_grpc/grpc/grpc_server_test_base.h"

#include <data_relay_grpc/blob_relay/service.h>
#include "data_relay_grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::common {

class session_store_directory_basic_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    const std::uint64_t tag_for_test = 2468;

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("session_store_directory_basic_test")};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
    }

    void TearDown() override {
        helper_->tear_down();
        data_relay_grpc::grpc::grpc_server_test_base::TearDown();
    }

    api api_for_test{
        [this](std::uint64_t bid, std::uint64_t tid) {
            return tag_for_test;
        },
        [this](std::uint64_t bid){
            return helper_->last_path();
        }
    };

    std::unique_ptr<blob_relay::blob_relay_service> service_{};
};

TEST_F(session_store_directory_basic_test, basic) {
    blob_relay::service_configuration conf_for_test {
        helper_->path(),  // session_store
        0,                // session_quota_size
        false,            // local_enabled
        false,            // local_upload_copy_file
        32,               // stream_chunk_size
        false             // dev_accept_mock_tag
    };

    EXPECT_NO_THROW( { service_ = std::make_unique<blob_relay::blob_relay_service>(api_for_test, conf_for_test); } );
}

} // namespace
