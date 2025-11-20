#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <exception>

#include "test_root.h"
#include "data-relay-grpc/grpc/grpc_server_test_base.h"

#include <data-relay-grpc/blob_relay/service.h>
#include "data-relay-grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::blob_relay {

class session_store_directory_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    const std::uint64_t tag_for_test = 2468;

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("session_store_directory_test")};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
    }

    void TearDown() override {
        helper_->tear_down();
        data_relay_grpc::grpc::grpc_server_test_base::TearDown();
    }

    blob_relay_service::api api_for_test{
        [this](std::uint64_t bid, std::uint64_t tid) {
            return tag_for_test;
        },
        [this](std::uint64_t bid){
            return helper_->last_path();
        }
    };

    std::unique_ptr<blob_relay_service> service_{};
};

TEST_F(session_store_directory_test, basic) {
    service_configuration conf_for_test {
        helper_->path(),  // session_store
        0,                // session_quota_size
        false,            // local_enabled
        false,            // local_upload_copy_file
        32                // stream_chunk_size
    };

    EXPECT_NO_THROW( { service_ = std::make_unique<blob_relay_service>(api_for_test, conf_for_test); } );
}

TEST_F(session_store_directory_test, symbolic_link) {
    auto d = helper_->path("dir");
    std::filesystem::create_directory(d);

    auto l = helper_->path("link");
    std::filesystem::create_symlink(d, l);

    service_configuration conf_for_test {
        l,      // session_store (symbolik link to a directory)
        0,      // session_quota_size
        false,  // local_enabled
        false,  // local_upload_copy_file
        32      // stream_chunk_size
    };

    EXPECT_NO_THROW( { service_ = std::make_unique<blob_relay_service>(api_for_test, conf_for_test); } );
}

TEST_F(session_store_directory_test, not_exist) {
    service_configuration conf_for_test {
        helper_->path("dir"), // session_store
        0,                    // session_quota_size
        false,                // local_enabled
        false,                // local_upload_copy_file
        32                    // stream_chunk_size
    };

    EXPECT_THROW( { service_ = std::make_unique<blob_relay_service>(api_for_test, conf_for_test); }, std::runtime_error );
}

TEST_F(session_store_directory_test, not_directory) {
    auto f = helper_->path("file");
    std::ofstream strm(f);
    strm << "test data\n";
    strm.close();

    service_configuration conf_for_test {
        f,      // session_store
        0,      // session_quota_size
        false,  // local_enabled
        false,  // local_upload_copy_file
        32      // stream_chunk_size
    };

    EXPECT_THROW( { service_ = std::make_unique<blob_relay_service>(api_for_test, conf_for_test); }, std::runtime_error );
}

} // namespace
