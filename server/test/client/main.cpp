#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <csignal>
#include <filesystem>
#include <atomic>

#include <gflags/gflags.h>

#include "client.h"
#include "session.h"

DEFINE_string(dbname, "tsurugi", "the database name");
DEFINE_int32(threads, 0, "the number of test threads");
DEFINE_bool(loop, false, "loop the put operation forever");
DEFINE_int64(put_size, -1, "blob size for Put operarion");
DEFINE_int64(get_size, -1, "blob size for Get operarion");
DEFINE_bool(vervose, false, "print vervose log");
DEFINE_uint32(sleep, 0, "sleep before session close");
DEFINE_uint32(fault, 0, "fault mode");

static std::vector<std::thread> threads{};
static std::atomic_uint64_t job_id{};

static void signal_handler([[maybe_unused]] int sig) {
    FLAGS_loop = false;
}

using namespace std::literals::string_literals;
const static std::string server_address = "localhost:52345"s;

void put(std::uint64_t size) {
    do {
        data_relay_grpc::blob_relay::session session(server_address);
        data_relay_grpc::blob_relay::Client client(server_address, session.session_id());

        auto blob_id = client.put(size);
        auto path = session.file_path(blob_id);
        if (!client.compare(path)) {
            throw std::runtime_error("inconsistent file contents");
        }
    } while (FLAGS_loop);
}

void get(std::uint64_t size, std::uint64_t jid) {
    do {
        data_relay_grpc::blob_relay::session session(server_address, jid);
        data_relay_grpc::blob_relay::Client client(server_address, session.session_id());

        auto pair = session.create_blob_file_for_download(size);
        if (FLAGS_fault == 4) {  // fault 4
            std::filesystem::remove(session.file_path(pair.first));
        }
        std::string path("download_");
        path += std::to_string(jid);
        client.get(pair.first, pair.second, path);
        if (!client.compare(path)) {
            throw std::runtime_error("inconsistent file contents");
        }
        std::filesystem::remove(path);
    } while (FLAGS_loop);
}

void do_exam() {
    try {
        if (FLAGS_put_size >= 0) {
            put(FLAGS_put_size);
        }
        if (FLAGS_get_size >= 0) {
            get(FLAGS_get_size, ++job_id);
        }
    } catch (std::runtime_error &ex) {
        std::cerr << "runtime_error: " << ex.what() << std::endl;
    } catch (std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    gflags::SetUsageMessage("tateyama gRPC relay test program");
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);

    if (signal(SIGINT, signal_handler) == SIG_ERR) {  // NOLINT  #define SIG_ERR  ((__sighandler_t) -1) in a system header file
        std::cerr << "cannot register signal handler" << std::endl;
        exit(1);
    }

    if (FLAGS_threads > 1) {
        threads.reserve(FLAGS_threads);
        for (int i = 0; i < FLAGS_threads; i++) {
            threads.emplace_back(std::thread([&](){ do_exam(); }));
        }
        for (std::thread &th : threads) {
            if (th.joinable()) {
                th.join();
            }
        }
    } else {
        do_exam();
    }
    return 0;
}
