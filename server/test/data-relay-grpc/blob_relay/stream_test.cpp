#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <exception>

#include "test_root.h"
#include "data-relay-grpc/grpc/grpc_server_test_base.h"

#include <data-relay-grpc/blob_relay/services.h>
#include "data-relay-grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::blob_relay {

class stream_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    const std::string test_partial_blob{"ABCDEFGHIJKLMNOPQRSTUBWXYZabcdefghijklmnopqrstubwxyz\n"};
    const std::string session_store_name{"session_store"};
    const std::uint64_t transaction_id_for_test = 12345;
    const std::uint64_t blob_id_for_test = 6789;
    const std::uint64_t tag_for_test = 2468;

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("stream_test")};
    blob_session* session_{};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
        std::filesystem::create_directory(helper_->path(session_store_name));
        services_ = std::make_unique<services>(
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
            services_->add_blob_relay_services(builder);
        });
        session_ = &services_->create_session(transaction_id_for_test);
    }

    void TearDown() override {
        helper_->tear_down();
        data_relay_grpc::grpc::grpc_server_test_base::TearDown();
    }

    void set_blob_data() {
        std::ofstream strm;
        strm.open(helper_->path("blob_data"));
        for (int i = 0; i < 10; i++ ) {
            strm << test_partial_blob;
        }
        strm.close();
    }

    blob_session_manager& get_session_manager() {
        return  services_->get_session_manager();
    }

private:
    services::api api_for_test{
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

    std::unique_ptr<services> services_{};
    std::uint64_t session_id_{};
    std::uint64_t transaction_id_{};
    std::uint64_t blob_id_{};
};

TEST_F(stream_test, get) {
    start_server();
    set_blob_data();
    
    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_session_id(session_->session_id());
    auto* blob = req.mutable_blob();
    blob->set_object_id(blob_id_for_test);
    blob->set_tag(tag_for_test);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    std::string blob_data{};
    while (reader->Read(&resp)) {
        blob_data += resp.chunk();
    }
    ::grpc::Status status = reader->Finish();

    std::ifstream ifs(helper_->last_path());
    std::string s{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    EXPECT_EQ(blob_data, s);
}

TEST_F(stream_test, put) {
    start_server();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
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
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::OK);
    // send blob data end

    auto& session_manager = get_session_manager();
    try {
        auto& session_impl = session_manager.get_session_impl(session_->session_id());
        if (auto path = session_impl.find(res.blob().object_id()); path) {
            std::ifstream ifs(path.value());
            std::string s{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
            EXPECT_EQ(ss.str(), s);
        } else {
            FAIL();
        }
    } catch (std::runtime_error &ex) {
        FAIL();
    }
}

} // namespace
