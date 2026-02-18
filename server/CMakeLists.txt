# Copyright 2022-2026 Project Tsurugi.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(data-relay-grpc
    VERSION 1.0.0
    DESCRIPTION "data-relay-grpc datastore engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option(ENABLE_SANITIZER "enable sanitizer on debug build" ON)
option(ENABLE_UB_SANITIZER "enable undefined behavior sanitizer on debug build" OFF)
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_DOCUMENTS "build documents" ON)
option(BUILD_STRICT "build with option strictly determine of success" ON)
option(BUILD_SHARED_LIBS "build shared libraries instead of static" ON)
option(USE_GRPC_CONFIG "Use CMake Config mode for gRPC instead of pkg-config" OFF)
option(SMOKE_TEST_SUPPORT "Include smoke test support functionalities" OFF)

if (FORCE_INSTALL_RPATH)
    message(DEPRECATION "FORCE_INSTALL_RPATH is obsoleted")
endif (FORCE_INSTALL_RPATH)

if (ENABLE_GOOGLE_PERFTOOLS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_GOOGLE_PERFTOOLS")
endif()

set(RECOVERY_SORTER_KVSLIB ROCKSDB CACHE STRING "using eKVS library at recovery process")
string(TOUPPER $CACHE{RECOVERY_SORTER_KVSLIB} RECOVERY_SORTER_KVSLIB_UPPERCASE)
option(RECOVERY_SORTER_PUT_ONLY "using put-only method at recovery process (faster)" ON)

find_package(Doxygen)
find_package(glog REQUIRED)
find_package(gflags REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost
        COMPONENTS filesystem thread system container
        REQUIRED
        )
find_package(OpenSSL REQUIRED)

# gRPC/protobuf
find_package(Protobuf REQUIRED)
if(USE_GRPC_CONFIG)
    message(STATUS "Using gRPC CMake Config mode")
    find_package(gRPC CONFIG REQUIRED)
else()
    message(STATUS "Using gRPC pkg-config mode")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPC REQUIRED grpc++)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CompileOptions)
include(InstallOptions)
include(Tests)

add_subdirectory(third_party) # should be before enable_testing()

if (BUILD_TESTS OR BUILD_EXAMPLES)
    enable_testing()
endif()

set(export_name "data-relay-grpc")
set(package_name "data-relay-grpc")

add_library(data-relay-grpc-api INTERFACE)

target_include_directories(data-relay-grpc-api
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/data-relay-grpc>
)

add_subdirectory(src)
if(BUILD_TESTS)
    add_subdirectory(test)
endif()
# if(BUILD_EXAMPLES)
#     add_subdirectory(examples)
# endif()
# if (BUILD_DOCUMENTS)
#     add_subdirectory(doxygen)
# endif()

install(
    TARGETS
        data-relay-grpc-api
    EXPORT
        ${export_name}
    LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT Runtime
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${export_name}
        COMPONENT Development
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime
)


install(
    FILES ${SCRIPTS}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

install(
    DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${export_name}"
    COMPONENT Development
    FILES_MATCHING PATTERN "*.h"
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${package_name}-config.cmake
        @ONLY
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${package_name}-config-version.cmake"
        COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${package_name}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${package_name}-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/${package_name}
)

install(
    EXPORT ${package_name}
    FILE ${package_name}-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${package_name}
    EXPORT_LINK_INTERFACE_LIBRARIES
)

