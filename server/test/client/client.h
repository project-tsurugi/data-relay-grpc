#pragma once

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>

#include <gflags/gflags.h>

#include "blob_relay_streaming.grpc.pb.h"

DECLARE_uint64(put_size);
DECLARE_bool(vervose);

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace data_relay_grpc::blob_relay {

class Client {
    constexpr static std::size_t buffer_size = 32;

    class test_blob {
    public:
        test_blob() : size_(ref_.size()) {
        }
        bool compare(const std::string& data, std::size_t s) {
            if (s < 1) {
                return false;
            }

            bool rv = false;
            std::size_t e = p_ + s;
            if ((p_ / size_) == ((e - 1) / size_)) {
                rv = ref_.substr(p_ % size_, s).compare(data.substr(0, s)) == 0;
            } else {
                std::size_t b = p_ % size_;
                std::size_t rem = size_ - b;

                if (ref_.substr(b, rem).compare(data.substr(0, rem)) == 0) {
                    rv = ref_.substr(0, s - rem).compare(data.substr(rem)) == 0;
                }
            }
            p_ += s;
            return rv;
        }
        std::string& chunk() {
            return ref_;
        }
        std::size_t length() {
            return ref_.length();
        }

    private:
        // ref_ should be the same as data_relay_grpc::blob_relay::smoke_test::smoketest_support_service::ref_
        std::string ref_{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\n"};
        std::size_t size_;
        std::size_t p_{0};
    };

public:
    Client(const std::string& server_address, std::size_t session_id) : server_address_(server_address), session_id_(session_id) {
    }

    std::uint64_t put() {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        proto::BlobRelayStreaming::Stub stub(channel);
        ::grpc::ClientContext context;

        std::unique_ptr<::grpc::ClientWriter<proto::PutStreamingRequest> > writer(stub.Put(&context, &res_));

        // send metadata
        proto::PutStreamingRequest req_metadata;
        auto* metadata = req_metadata.mutable_metadata();
        metadata->set_session_id(session_id_);
        if (!writer->Write(req_metadata)) {
            throw std::runtime_error(std::string("error in ") + __func__ + " at " + std::to_string(__LINE__));
        }

        // send blob data begin
        proto::PutStreamingRequest req_chunk;
        std::size_t transfered_size{};
        while (transfered_size < FLAGS_put_size) {
            std::size_t size = std::min(reference_.length(), FLAGS_put_size - transfered_size);
            req_chunk.set_chunk(reference_.chunk().data(), size);
            transfered_size += size;
            if (!writer->Write(req_chunk)) {
                throw std::runtime_error(std::string("error in ") + __func__ + " at " + std::to_string(__LINE__));
            }
        }
        writer->WritesDone();
        ::grpc::Status status = writer->Finish();
        if (status.error_code() != ::grpc::StatusCode::OK) {
            throw std::runtime_error(std::string("error in ") + __func__ + " at " + std::to_string(__LINE__));
        }
        return res_.blob().object_id();
    }

    void get(std::uint64_t blob_id, std::uint64_t tag, std::filesystem::path path) {
        auto channel = ::grpc::CreateChannel(server_address_, ::grpc::InsecureChannelCredentials());
        proto::BlobRelayStreaming::Stub stub(channel);
        ::grpc::ClientContext context;
        proto::GetStreamingRequest req;
        req.set_session_id(session_id_);
        auto* blob = req.mutable_blob();
        blob->set_object_id(blob_id);
        blob->set_tag(tag);
        std::unique_ptr<::grpc::ClientReader<proto::GetStreamingResponse> > reader(stub.Get(&context, req));

        proto::GetStreamingResponse resp;
        std::ofstream blob_file(path);
        if (!blob_file) {
            throw std::runtime_error(std::string("error in ") + __func__ + " at " + std::to_string(__LINE__));
        }
        while (reader->Read(&resp)) {
            auto& chunk = resp.chunk();
            blob_file.write(chunk.data(), chunk.length());
        }
        ::grpc::Status status = reader->Finish();
        if (status.error_code() != ::grpc::StatusCode::OK) {
            throw std::runtime_error(std::string("error in ") + __func__ + " at " + std::to_string(__LINE__));
        }
    }
    
    bool compare(std::filesystem::path const path) {
        std::ifstream blob_file(path);
        if (!blob_file) {
            return false;
        }

        blob_file.seekg(0, std::ios::end);
        std::streamsize fileSize = blob_file.tellg();
        blob_file.seekg(0, std::ios::beg);

        std::string buffer{};
        buffer.resize(buffer_size);

        while (blob_file.tellg() < fileSize) {
            auto size = std::min(static_cast<std::size_t>(fileSize - blob_file.tellg()), buffer_size);
            blob_file.read(reinterpret_cast<char*>(buffer.data()), size);
            if (FLAGS_vervose) {
                std::cout << "compare: " << buffer.substr(0, size) << std::endl;
            }
            if (!reference_.compare(buffer, size)) {
                return false;
            }
        }
        return true;
    }

private:
    std::string server_address_;
    std::size_t session_id_;
    std::unique_ptr<proto::BlobRelayStreaming::Stub> stub_;
    proto::PutStreamingResponse res_;
    test_blob reference_{};
};

}  // namespace
