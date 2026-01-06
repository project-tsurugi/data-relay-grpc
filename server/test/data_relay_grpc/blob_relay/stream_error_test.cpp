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
#include <data_relay_grpc/blob_relay/api_version.h>
#include "data_relay_grpc/blob_relay/streaming_service.h"

namespace data_relay_grpc::blob_relay {

class stream_error_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    const std::string test_partial_blob{"ABCDEFGHIJKLMNOPQRSTUBWXYZabcdefghijklmnopqrstubwxyz\n"};
    const std::string session_store_name{"session_store"};
    const std::uint64_t transaction_id_for_test = 12345;
    const std::uint64_t tag_for_test = 2468;
    std::uint64_t blob_id_for_test{};

    std::unique_ptr<directory_helper> helper_{std::make_unique<directory_helper>("stream_error_test")};
    blob_session* session_{};

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
        helper_->set_up();
        std::filesystem::create_directory(helper_->path(session_store_name));
        service_ = std::make_unique<blob_relay_service_impl>(
            api_for_test,
            service_configuration{
                helper_->path(session_store_name),  // session_store
                0,                                  // session_quota_size
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

    blob_session_manager& get_session_manager() {
        return  service_->get_session_manager();
    }

private:
    blob_relay_service::api api_for_test{
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

    std::unique_ptr<blob_relay_service_impl> service_{};
    std::uint64_t session_id_{};
    std::uint64_t transaction_id_{};
    std::atomic_uint64_t blob_id_{};
};

TEST_F(stream_error_test, get_ok) {
    start_server();
    set_blob_data();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_api_version(BLOB_RELAY_API_VERSION);
    req.set_session_id(session_->session_id());
    auto* blob = req.mutable_blob();
    blob->set_object_id(blob_id_for_test);
    blob->set_tag(tag_for_test);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    if (!reader->Read(&resp)) {
        FAIL();
    }
    if (resp.payload_case() != GetStreamingResponse::PayloadCase::kMetadata) {
        FAIL();
    }
    std::size_t blob_size = resp.metadata().blob_size();  // streaming_service always set blob_size

    std::string blob_data{};
    while (reader->Read(&resp)) {
        if (resp.payload_case() != GetStreamingResponse::PayloadCase::kChunk) {
            FAIL();
        }
        blob_data += resp.chunk();
    }
    ::grpc::Status status = reader->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::OK);
    EXPECT_EQ(blob_data.size(), blob_size);
}

TEST_F(stream_error_test, get_api_version) {
    start_server();
    set_blob_data();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_api_version(BLOB_RELAY_API_VERSION + 1);  // not compatible
    req.set_session_id(session_->session_id());
    auto* blob = req.mutable_blob();
    blob->set_object_id(blob_id_for_test);
    blob->set_tag(tag_for_test);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    if (reader->Read(&resp)) {
        FAIL();
    }
    ::grpc::Status status = reader->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::UNAVAILABLE);
    std::cerr << status.error_message () << std::endl;
}

TEST_F(stream_error_test, get_invalid_tag) {
    start_server();
    set_blob_data();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_api_version(BLOB_RELAY_API_VERSION);
    req.set_session_id(session_->session_id());
    auto* blob = req.mutable_blob();
    blob->set_object_id(blob_id_for_test);
    blob->set_tag(tag_for_test + 1);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    if (reader->Read(&resp)) {
        FAIL();
    }
    ::grpc::Status status = reader->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::PERMISSION_DENIED);
}

TEST_F(stream_error_test, get_no_session) {
    start_server();
    set_blob_data();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_api_version(BLOB_RELAY_API_VERSION);
    req.set_session_id(session_->session_id() + 1);  // set sessoin_id that does not exist
    auto* blob = req.mutable_blob();
    blob->set_object_id(blob_id_for_test);
    blob->set_tag(tag_for_test);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    if (reader->Read(&resp)) {
        FAIL();
    }
    ::grpc::Status status = reader->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::NOT_FOUND);
}

TEST_F(stream_error_test, get_no_blob_file) {
    start_server();
    set_blob_data();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;
    GetStreamingRequest req;
    req.set_api_version(BLOB_RELAY_API_VERSION);
    req.set_session_id(session_->session_id());
    auto* blob = req.mutable_blob();
    blob->set_object_id(0);  // not exists
    blob->set_tag(tag_for_test);
    std::unique_ptr<::grpc::ClientReader<GetStreamingResponse> > reader(stub.Get(&context, req));

    GetStreamingResponse resp;
    if (reader->Read(&resp)) {
        FAIL();
    }
    ::grpc::Status status = reader->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::NOT_FOUND);
}

TEST_F(stream_error_test, put_ok) {
    start_server();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
    std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

    // send metadata
    PutStreamingRequest req_metadata;
    auto* metadata = req_metadata.mutable_metadata();
    metadata->set_api_version(BLOB_RELAY_API_VERSION);
    metadata->set_session_id(session_->session_id());
    metadata->set_blob_size(test_partial_blob.size() * 10);
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
}

TEST_F(stream_error_test, put_api_version) {
    start_server();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
    std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

    // send metadata
    PutStreamingRequest req_metadata;
    auto* metadata = req_metadata.mutable_metadata();
    metadata->set_api_version(BLOB_RELAY_API_VERSION + 1);  // not compatible
    metadata->set_session_id(session_->session_id());
    metadata->set_blob_size(test_partial_blob.size() * 10);
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
            // error
            break;
        }
    }
    writer->WritesDone();
    ::grpc::Status status = writer->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::UNAVAILABLE);
    std::cerr << status.error_message () << std::endl;
    // send blob data end
}

TEST_F(stream_error_test, put_no_metadata) {
    start_server();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
    std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

    // send blob data begin
    PutStreamingRequest req_chunk;
    std::stringstream ss{};
    for (int i = 0; i < 10; i++) {
        req_chunk.set_chunk(test_partial_blob);
        ss << test_partial_blob;
        if (!writer->Write(req_chunk)) {
            // error
            break;
        }
    }
    writer->WritesDone();
    ::grpc::Status status = writer->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::INVALID_ARGUMENT);
    // send blob data end
}

TEST_F(stream_error_test, put_no_session) {
    start_server();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
    std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

    // send metadata
    PutStreamingRequest req_metadata;
    auto* metadata = req_metadata.mutable_metadata();
    metadata->set_api_version(BLOB_RELAY_API_VERSION);
    metadata->set_blob_size(test_partial_blob.size() * 10);
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
            // error
            break;
        }
    }
    writer->WritesDone();
    ::grpc::Status status = writer->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::NOT_FOUND);
    // send blob data end
}

TEST_F(stream_error_test, put_blob_size_mismatch) {
    start_server();

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
    std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

    // send metadata
    PutStreamingRequest req_metadata;
    auto* metadata = req_metadata.mutable_metadata();
    metadata->set_api_version(BLOB_RELAY_API_VERSION);
    metadata->set_session_id(session_->session_id());
    metadata->set_blob_size(test_partial_blob.size() * 10 + 1);
    if (!writer->Write(req_metadata)) {
        FAIL();
    }

    writer->WritesDone();
    ::grpc::Status status = writer->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(stream_error_test, put_file_write_permission) {
    // root can write file to write-protected files
    if (geteuid() == 0) { GTEST_SKIP() << "skip when run by root"; }

    start_server();

    // the same as 'chmod -w directory_for_session_store'
    std::filesystem::perms pmask = std::filesystem::perms::owner_write
        | std::filesystem::perms::group_write | std::filesystem::perms::others_write;
    std::filesystem::permissions(helper_->path(session_store_name), pmask, std::filesystem::perm_options::remove);

    auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
    BlobRelayStreaming::Stub stub(channel);
    ::grpc::ClientContext context;

    PutStreamingResponse res;
    std::unique_ptr<::grpc::ClientWriter<PutStreamingRequest> > writer(stub.Put(&context, &res));

    // send metadata
    PutStreamingRequest req_metadata;
    auto* metadata = req_metadata.mutable_metadata();
    metadata->set_api_version(BLOB_RELAY_API_VERSION);
    metadata->set_session_id(session_->session_id());
    metadata->set_blob_size(test_partial_blob.size() * 10);
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
            // error
            break;
        }
    }
    writer->WritesDone();
    ::grpc::Status status = writer->Finish();
    EXPECT_EQ(status.error_code(), ::grpc::StatusCode::FAILED_PRECONDITION);
    // send blob data end
}

} // namespace
