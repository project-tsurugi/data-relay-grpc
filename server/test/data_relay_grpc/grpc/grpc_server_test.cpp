#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "grpc_server_test_base.h"

namespace data_relay_grpc::grpc {

class grpc_server_test : public data_relay_grpc::grpc::grpc_server_test_base {
protected:
    static constexpr const char* log_dir = "/tmp/grpc_server_test";

    void SetUp() override {
        data_relay_grpc::grpc::grpc_server_test_base::SetUp();
    }

    void TearDown() override {
        data_relay_grpc::grpc::grpc_server_test_base::TearDown();
    }
};

TEST_F(grpc_server_test, basec_function) {
    start_server();
}

} // namespace
