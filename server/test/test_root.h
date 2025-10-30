/*
 * Copyright 2025-2025 Project Tsurugi.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <thread>
#include <chrono>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include <filesystem>
#include <string>

class directory_helper {
  public:
    explicit directory_helper(std::string prefix) : prefix_(prefix) {
        location_ = std::filesystem::path(testing::TempDir());
        location_ /= std::filesystem::path(prefix_ + std::to_string(getpid()));
    }

    void set_up() {
        try {
            std::filesystem::remove_all(location_);       
            std::filesystem::create_directory(location_);       
        } catch (std::filesystem::filesystem_error &ex) {
            FAIL();
        }
    }

    void tear_down() {
        try {
            std::filesystem::remove_all(location_);       
        } catch (std::filesystem::filesystem_error &ex) {
            std::cerr << ex.what() << ":" << location_.string() << std::endl;
            FAIL();
        }
    }

    std::filesystem::path path(const std::string& child) {
        last_ = location_ / std::filesystem::path(child);
        return last_;
    }

    std::filesystem::path path() {
        last_ = location_;
        return last_;
    }

    std::filesystem::path last_path() {
        return last_;
    }

private:
    std::string prefix_;
    std::filesystem::path location_{};
    std::filesystem::path last_{};
};
