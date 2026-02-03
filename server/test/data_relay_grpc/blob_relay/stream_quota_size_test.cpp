#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <exception>

#include "test_root.h"
#include "data_relay_grpc/grpc/grpc_server_test_base.h"

#include "data_relay_grpc/blob_relay/service_impl.h"
#include <data_relay_grpc/blob_relay/api_version.h>
#include "data_relay_grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::blob_relay {

class stream_quota_size_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    const std::string test_partial_blob{"ABCDEFGHIJKLMNOPQRSTUBWXYZabcdefghijklmnopqrstubwxyz\n"};
    const std::string session_store_name{"session_store"};
    const std::uint64_t quota_size_for_test = 4096;
    const std::uint64_t transaction_id_for_test = 12345;
    const std::uint64_t blob_id_for_test = 6789;
    const std::uint64_t tag_for_test = 2468;
    const std::uint64_t loop_count = 10;

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("stream_quota_size_test")};
    blob_session* session_{};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
        std::filesystem::create_directory(helper_->path(session_store_name));
        service_ = std::make_unique<blob_relay_service>(
            api_for_test,
            service_configuration{
                helper_->path(session_store_name),  // session_store
                quota_size_for_test,                // session_quota_size
                false,                              // local_enabled
                false,                              // local_upload_copy_file
                32,                                 // stream_chunk_size
                false                               // dev_accept_mock_tag
            }
        );
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

    ::grpc::Status send_blob(PutStreamingResponse& res) {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        BlobRelayStreaming::Stub stub(channel);
        ::grpc::ClientContext context;

        std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

        // send metadata
        PutStreamingRequest req_metadata;
        auto* metadata = req_metadata.mutable_metadata();
        metadata->set_api_version(BLOB_RELAY_API_VERSION);
        metadata->set_session_id(session_->session_id());
        metadata->set_blob_size(test_partial_blob.size() * loop_count);
        if (!writer->Write(req_metadata)) {
            throw std::runtime_error("test failed");
        }

        // send blob data begin
        PutStreamingRequest req_chunk;
        std::stringstream ss{};
        for (int i = 0; i < loop_count; i++) {
            req_chunk.set_chunk(test_partial_blob);
            ss << test_partial_blob;
            if (!writer->Write(req_chunk)) {
                throw std::runtime_error("test failed");
            }
        }
        writer->WritesDone();
        return writer->Finish();
    }

    std::size_t file_count() {
        std::size_t rv{0};
        for (const std::filesystem::directory_entry& e : std::filesystem::directory_iterator(helper_->path(session_store_name))) {
            rv++;
            // std::cout << e.path() << std::endl;
        }
        return rv;
    }

    common::blob_session_manager& get_session_manager() {
        return  service_->impl().get_session_manager();
    }

    std::size_t session_store_current_usage() {
        return get_session_manager().session_store_current_size();
    }

private:
    common::api api_for_test{
        [this](std::uint64_t bid, std::uint64_t tid) {
            return tag_for_test;
        },
        [this](std::uint64_t bid){
            return helper_->last_path();
        }
    };

    std::unique_ptr<blob_relay_service> service_{};
    std::uint64_t session_id_{};
    std::uint64_t transaction_id_{};
    std::uint64_t blob_id_{};
};

TEST_F(stream_quota_size_test, quota_size) {
    start_server();

    auto blob_size = loop_count * test_partial_blob.length();
    std::vector<PutStreamingResponse> res{quota_size_for_test / blob_size};
    std::uint64_t blob_count{0};
    for (std::uint64_t l = 0; l < (quota_size_for_test - blob_size); l += blob_size) {
        auto status = send_blob(res.at(blob_count));
        try {
            EXPECT_EQ(status.error_code(), ::grpc::StatusCode::OK);
        } catch (std::runtime_error &ex) {
            std::cerr << ex.what() << std::endl;
            FAIL();
        }
        blob_count++;
    }
    EXPECT_EQ(blob_count, file_count());

    auto& session_impl = get_session_manager().get_session_impl(session_->session_id());
    EXPECT_EQ(blob_count * blob_size, session_store_current_usage());

    std::vector<blob_session::blob_id_type> entries = session_->entries();
    EXPECT_EQ(entries.size(), file_count());

    for (auto&& e: entries) {
        auto path_opt = session_->find(e);
        ASSERT_TRUE(path_opt);
        std::filesystem::remove(path_opt.value());
    }
    EXPECT_EQ(0, file_count());
    EXPECT_EQ(blob_count * blob_size, session_store_current_usage());

    EXPECT_NO_THROW({ session_->remove(entries.begin(), entries.end()); });
    EXPECT_EQ(0, file_count());
    EXPECT_EQ(0, session_store_current_usage());
}

} // namespace
