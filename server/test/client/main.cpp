#include <iostream>
#include <stdexcept>
#include <gflags/gflags.h>

#include "client.h"
#include "session.h"

DEFINE_string(dbname, "tsurugi", "the database name");

int main(int argc, char** argv) {
    try {
        gflags::SetUsageMessage("tateyama gRPC relay test program");
        gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
        
        tateyama::session session{};
        data_relay_grpc::blob_relay::Client client("localhost:50051", session.session_id());
        client.put();
        return 0;
    } catch (std::runtime_error &ex) {
        std::cerr << ex.what() << std::endl;
    }
}
