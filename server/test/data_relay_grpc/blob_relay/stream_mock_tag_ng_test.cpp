#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <atomic>
#include <exception>

#include "test_root.h"
#include "data_relay_grpc/grpc/grpc_server_test_base.h"

#include "data_relay_grpc/blob_relay/service_impl.h"
#include "data_relay_grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::blob_relay {

class stream_mock_tag_ng_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    const std::string test_partial_blob{"ABCDEFGHIJKLMNOPQRSTUBWXYZabcdefghijklmnopqrstubwxyz\n"};
    const std::string session_store_name{"session_store"};
    const std::uint64_t transaction_id_for_test = 12345;
    const std::uint64_t tag_for_test = 2468;
    const std::uint64_t api_version = 0;
    std::uint64_t blob_id_for_test{};

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("stream_mock_tag_ng_test")};
    blob_session* session_{};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
        std::filesystem::create_directory(helper_->path(session_store_name));
    }
    void PostSetUp() {
        set_service_handler([this](::grpc::ServerBuilder& builder) {
            for(auto&& e: service_->services()) {
                builder.RegisterService(e);
            }
        });
        session_ = &service_->create_session(transaction_id_for_test);
    }

    void TearDown() override {
        helper_->tear_down();
        data_relay_grpc::grpc::grpc_server_test_base::TearDown();
    }

    void set_blob_data() {
        std::filesystem::path path = helper_->path(std::string("blob-") + std::to_string(++blob_id_));
        std::ofstream strm(path);
        if (!strm) {
            FAIL();
        }
        for (int i = 0; i < 10; i++ ) {
            strm << test_partial_blob;
        }
        strm.close();
        blob_id_for_test = session_->add(path);
    }

    common::blob_session_manager& get_session_manager() {
        return  service_->impl().get_session_manager();
    }

protected:
    common::api api_for_test{
        [this](std::uint64_t bid, std::uint64_t tid) {
            return tag_for_test;
        },
        [this](std::uint64_t bid){
            if (bid == blob_id_for_test) {
                return helper_->last_path();
            }
            return std::filesystem::path{};
        }
    };
    std::unique_ptr<blob_relay_service> service_{};

private:
    std::uint64_t session_id_{};
    std::uint64_t transaction_id_{};
    std::atomic_uint64_t blob_id_{};
};

TEST_F(stream_mock_tag_ng_test, get_with_mocktag_ng) {
    service_ = std::make_unique<blob_relay_service>(
        api_for_test,
        service_configuration {
            helper_->path(session_store_name),  // session_store
            0,                                  // session_quota_size
            false,                              // local_enabled
            false,                              // local_upload_copy_file
            32,                                 // stream_chunk_size
            false                               // dev_accept_mock_tag
        });
    PostSetUp();
    start_server();
    set_blob_data();
    
    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_api_version(api_version);
    req.set_session_id(session_->session_id());
    auto* blob = req.mutable_blob();
    blob->set_object_id(blob_id_for_test);
    blob->set_tag(common::blob_session_manager::MOCK_TAG);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    if (reader->Read(&resp)) {
        FAIL();
    }
    ::grpc::Status status = reader->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::PERMISSION_DENIED);
}

} // namespace
