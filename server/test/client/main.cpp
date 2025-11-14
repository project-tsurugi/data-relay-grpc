#include <iostream>
#include <stdexcept>
#include <csignal>
#include <filesystem>

#include <gflags/gflags.h>

#include "client.h"
#include "session.h"

DEFINE_string(dbname, "tsurugi", "the database name");
DEFINE_int32(threads, 0, "the number of test threads");
DEFINE_bool(dispose, true, "dispose the put blob file at the end of the session");
DEFINE_bool(loop, false, "loop the put operation forever");
DEFINE_uint64(put_size, 0, "blob size for Put operarion");
DEFINE_uint64(get_size, 0, "blob size for Get operarion");
DEFINE_bool(vervose, false, "print vervose log");

static std::vector<std::thread> threads{};

static void signal_handler([[maybe_unused]] int sig) {
    FLAGS_loop = false;
}

void put() {
    while (true) {
        std::string server_address{"localhost:50051"};
        data_relay_grpc::blob_relay::session session(server_address);
        data_relay_grpc::blob_relay::Client client(server_address, session.session_id());

        auto blob_id = client.put();
        auto path = session.file_path(blob_id);
        if (!client.compare(path)) {
            throw std::runtime_error("inconsistent file contents");
        }
        if (FLAGS_loop) {
            continue;
        }
        break;
    }
}

void get(std::uint64_t size, std::uint64_t tid) {
    do {
        std::string server_address{"localhost:50051"};
        data_relay_grpc::blob_relay::session session(server_address, tid);
        data_relay_grpc::blob_relay::Client client(server_address, session.session_id());

        auto pair = session.create_blob_file_for_download(size);
        std::string path("download_");
        path += std::to_string(tid);
        client.get(pair.first, pair.second, path);
        if (!client.compare(path)) {
            throw std::runtime_error("inconsistent file contents");
        }
        std::filesystem::remove(path);
    } while (false);
}

int main(int argc, char** argv) {
    std::atomic_uint64_t job_id{};
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
            if (FLAGS_get_size == 0) {
                put();
            } else {
                get(FLAGS_get_size, ++job_id);
            }
        }
        return 0;
    } catch (std::runtime_error &ex) {
        std::cerr << "runtime_error: " << ex.what() << std::endl;
    } catch (std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
    }
}
