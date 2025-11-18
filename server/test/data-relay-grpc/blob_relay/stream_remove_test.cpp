#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <exception>

#include "test_root.h"
#include "data-relay-grpc/grpc/grpc_server_test_base.h"

#include <data-relay-grpc/blob_relay/service.h>
#include "data-relay-grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::blob_relay {

class stream_remove_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    static constexpr std::size_t ARRAY_SIZE = 100;
    const std::string session_store_name{"session_store"};
    const std::string test_partial_blob{"ABCDEFGHIJKLMNOPQRSTUBWXYZabcdefghijklmnopqrstubwxyz\n"};
    const std::uint64_t transaction_id_for_test = 12345;
    const std::uint64_t blob_id_for_test = 6789;
    const std::uint64_t tag_for_test = 2468;

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("stream_remove_test")};
    blob_session* session_{};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
        std::filesystem::create_directory(helper_->path(session_store_name));
        service_ = std::make_unique<blob_relay_service>(
            api_for_test,
            service_configuration{
                helper_->path(session_store_name),  // session_store
                0,                                  // session_quota_size
                false,                              // local_enabled
                false,                              // local_upload_copy_file
                32                                  // stream_chunk_size
            }
        );
        set_service_handler([this](::grpc::ServerBuilder& builder) {
            service_->add_blob_relay_service(builder);
        });
        session_ = &service_->create_session(transaction_id_for_test);
    }

    void TearDown() override {
        helper_->tear_down();
        data_relay_grpc::grpc::grpc_server_test_base::TearDown();
    }

    void send_blob(PutStreamingResponse& res) {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        BlobRelayStreaming::Stub stub(channel);
        ::grpc::ClientContext context;

        std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

        // send metadata
        PutStreamingRequest req_metadata;
        auto* metadata = req_metadata.mutable_metadata();
        metadata->set_session_id(session_->session_id());
        if (!writer->Write(req_metadata)) {
            FAIL();
        }

        // send blob data begin
        PutStreamingRequest req_chunk;
        std::stringstream ss{};
        for (int i = 0; i < 10; i++) {
            req_chunk.set_chunk(test_partial_blob);
            ss << test_partial_blob;
            if (!writer->Write(req_chunk)) {
                FAIL();
            }
        }
        writer->WritesDone();
        ::grpc::Status status = writer->Finish();
        // send blob data end
    }

    std::size_t file_count() {
        std::size_t rv{0};
        for (const std::filesystem::directory_entry& e : std::filesystem::directory_iterator(helper_->path(session_store_name))) {
            rv++;
            // std::cout << e.path() << std::endl;
        }
        return rv;
    }

    blob_session_manager& get_session_manager() {
        return  service_->get_session_manager();
    }

private:
    blob_relay_service::api api_for_test{
        [this](std::uint64_t bid, std::uint64_t tid) {
            EXPECT_EQ(bid, blob_id_for_test);
            EXPECT_EQ(tid, transaction_id_for_test);
            return tag_for_test;
        },
        [this](std::uint64_t bid){
            EXPECT_EQ(bid, blob_id_for_test);
            return helper_->last_path();
        }
    };
    std::unique_ptr<blob_relay_service> service_{};
    std::uint64_t session_id_{};
    std::uint64_t transaction_id_{};
    std::uint64_t blob_id_{};
};

TEST_F(stream_remove_test, vector) {
    start_server();

    std::array<PutStreamingResponse, ARRAY_SIZE> res{};
    std::vector<std::uint64_t> bids(ARRAY_SIZE);
    for (int i = 0; i < res.size(); i++) {
        send_blob(res.at(i));
        bids.at(i) = (res.at(i)).blob().object_id();
    }
    EXPECT_EQ(ARRAY_SIZE, file_count());

    auto& session_manager = get_session_manager();
    try {
        auto& session = session_manager.get_session(session_->session_id());
        session.remove(bids.begin(), bids.end());
        EXPECT_EQ(0, file_count());
    } catch (std::runtime_error &ex) {
        FAIL();
    }
}

TEST_F(stream_remove_test, vector_reverse) {
    start_server();

    std::array<PutStreamingResponse, ARRAY_SIZE> res{};
    std::vector<std::uint64_t> bids(ARRAY_SIZE);
    for (int i = 0; i < res.size(); i++) {
        send_blob(res.at(i));
        bids.at(i) = (res.at(i)).blob().object_id();
    }
    EXPECT_EQ(ARRAY_SIZE, file_count());

    auto& session_manager = get_session_manager();
    try {
        auto& session = session_manager.get_session(session_->session_id());
        auto itr = bids.begin();
        for (int i = 1; i < ARRAY_SIZE; i++) {
            itr++;
        }
        EXPECT_THROW({ session.remove(itr, bids.begin()); }, std::runtime_error);
        EXPECT_EQ(ARRAY_SIZE, file_count());
    } catch (std::runtime_error &ex) {
        FAIL();
    }
}

TEST_F(stream_remove_test, list) {
    start_server();

    std::array<PutStreamingResponse, ARRAY_SIZE> res{};
    std::list<std::uint64_t> bids{};
    for (int i = 0; i < res.size(); i++) {
        send_blob(res.at(i));
        bids.emplace_back((res.at(i)).blob().object_id());
    }
    EXPECT_EQ(ARRAY_SIZE, file_count());

    auto& session_manager = get_session_manager();
    try {
        auto& session = session_manager.get_session(session_->session_id());
        session.remove(bids.begin(), bids.end());
        EXPECT_EQ(0, file_count());
    } catch (std::runtime_error &ex) {
        FAIL();
    }
}

} // namespace
