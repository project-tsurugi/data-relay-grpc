#include <iostream>
#include <stdexcept>
#include <csignal>

#include <gflags/gflags.h>

#include "client.h"
#include "session.h"

DEFINE_string(dbname, "tsurugi", "the database name");
DEFINE_int32(threads, 0, "the number of test threads");
DEFINE_bool(dispose, true, "dispose the put blob file at the end of the session");
DEFINE_bool(loop, false, "loop the put operation forever");

static std::vector<std::thread> threads{};

static void signal_handler([[maybe_unused]] int sig) {
    FLAGS_loop = false;
}

void put() {
    while (true) {
        tateyama::session session{};
        data_relay_grpc::blob_relay::Client client("localhost:50051", session.session_id());

        client.put();
        if (FLAGS_loop) {
            continue;
        }
        break;
    }
}

int main(int argc, char** argv) {
    try {
        gflags::SetUsageMessage("tateyama gRPC relay test program");
        gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
        
        if (signal(SIGINT, signal_handler) == SIG_ERR) {  // NOLINT  #define SIG_ERR  ((__sighandler_t) -1) in a system header file
            std::cerr << "cannot register signal handler" << std::endl;
            exit(1);
        }

        if (FLAGS_threads > 1) {
            threads.reserve(FLAGS_threads);
            for (int i = 0; i < FLAGS_threads; i++) {
                threads.emplace_back(std::thread([&](){ put(); }));
            }
            for (std::thread &th : threads) {
                if (th.joinable()) {
                    th.join();
                }
            }
        } else {
            put();
        }
        return 0;
    } catch (std::runtime_error &ex) {
        std::cerr << ex.what() << std::endl;
    }
}
