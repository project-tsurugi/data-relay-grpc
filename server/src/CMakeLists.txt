file(GLOB SOURCES
        "data_relay_grpc/blob_relay/*.cpp"
        "data_relay_grpc/common/*.cpp"
        "data_relay_grpc/common/detail/*.cpp"
)

# find all .proto files in ${CMAKE_SOURCE_DIR}/../protos/data_relay_grpc/proto/blob_relay
file(REAL_PATH ${CMAKE_SOURCE_DIR}/../protos PROTO_DIR)
file(GLOB PROTO_FILES "${PROTO_DIR}/data_relay_grpc/proto/blob_relay/*.proto")

#
# Compile protobuf and grpc files in build_protos target to cpp
#
# Generate protobuf and gRPC files
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

############################################################################################################################
# add_custom_target(build_protos DEPENDS ${PROTO_FILES})
#
# protobuf_generate(TARGET build_protos LANGUAGE cpp OUT_VAR GENERATED_PROTO_FILES PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/src/data_relay_grpc/proto/blob_relay PROTOS ${PROTO_FILES} IMPORT_DIRS ${PROTO_DIR} APPEND_PATH ${PROTO_DIR})
# protobuf_generate(TARGET build_protos LANGUAGE grpc OUT_VAR GENERATED_GRPC_FILES PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/src/data_relay_grpc/proto/blob_relay PROTOS ${PROTO_FILES} IMPORT_DIRS ${PROTO_DIR} APPEND_PATH ${PROTO_DIR} GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}")
############################################################################################################################
# Ideally, you would like to compile with the above, but the following error occurs. A bug in protoc is suspected.
# data-relay-grpc/server/build/src/data_relay_grpc/proto/blob_relay/blob_relay_local.pb.cc:192:6: error: ‘::descriptor_table_data_5frelay_5fgrpc_2fproto_2fblob_5frelay_2fblob_5freference_2eproto’ has not been declared
# 192 |   &::descriptor_table_data_5frelay_5fgrpc_2fproto_2fblob_5frelay_2fblob_5freference_2eproto,
############################################################################################################################
add_custom_target(generated_protos DEPENDS ${PROTO_FILES})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/src/protos)
protobuf_generate(TARGET generated_protos LANGUAGE cpp OUT_VAR GENERATED_PROTO_FILES PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/src/protos PROTOS ${PROTO_FILES} IMPORT_DIRS ${PROTO_DIR})
protobuf_generate(TARGET generated_protos LANGUAGE grpc OUT_VAR GENERATED_GRPC_FILES PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/src/protos PROTOS ${PROTO_FILES} IMPORT_DIRS ${PROTO_DIR} GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}")

add_custom_target(build_protos DEPENDS generated_protos)
add_custom_command(
  TARGET build_protos
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/src/data_relay_grpc/proto/blob_relay ${CMAKE_BINARY_DIR}/protos/data_relay_grpc/proto/blob_relay
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/src/protos/data_relay_grpc/proto/blob_relay/*.pb.h" ${CMAKE_BINARY_DIR}/src/data_relay_grpc/proto/blob_relay
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/src/protos/data_relay_grpc/proto/blob_relay/*.pb.cc" ${CMAKE_BINARY_DIR}/protos/data_relay_grpc/proto/blob_relay
)
############################################################################################################################

set(PROTO_SRCS)
foreach(FILE ${GENERATED_PROTO_FILES})
  if(${FILE} MATCHES ".*\.cc")
    list(APPEND PROTO_SRCS ${FILE})
  endif()
endforeach()
foreach(FILE ${GENERATED_GRPC_FILES})
  if(${FILE} MATCHES ".*\.cc")
    list(APPEND PROTO_SRCS ${FILE})
  endif()
endforeach()

# Add generated files to sources
list(APPEND SOURCES ${PROTO_SRCS})

if(SMOKE_TEST_SUPPORT)
    list(APPEND SOURCES ${CMAKE_BINARY_DIR}/src/blob_relay_smoke_test.pb.cc ${CMAKE_BINARY_DIR}/src/blob_relay_smoke_test.grpc.pb.cc)
endif()

add_library(${package_name}
        ${SOURCES}
)

add_dependencies(${package_name}
        build_protos
)

set_target_properties(${package_name}
        PROPERTIES
                INSTALL_RPATH "\$ORIGIN"
                OUTPUT_NAME ${export_name}
)
target_include_directories(${package_name}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${package_name}
        PUBLIC data-relay-grpc-api
        PRIVATE Boost::boost
        PRIVATE Boost::filesystem
        PRIVATE Boost::thread
        PRIVATE Boost::container
        PRIVATE glog::glog
        PRIVATE ${sort_lib}
        PRIVATE grpc++
        PRIVATE grpc
        PRIVATE protobuf
        PRIVATE OpenSSL::Crypto
)

set_compile_options(${package_name})

install_custom(${package_name} ${export_name})

install(DIRECTORY
        ${CMAKE_BINARY_DIR}/src/data_relay_grpc
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/data-relay-grpc
        FILES_MATCHING
        PATTERN "proto/blob_relay/*.pb.h"
        )


# for tests
if(SMOKE_TEST_SUPPORT)
    set(SMOKETEST_PROTO_DIR ${CMAKE_SOURCE_DIR}/test/data_relay_grpc/blob_relay/smoke_test/proto)
    file(GLOB SMOKETEST_PROTO ${SMOKETEST_PROTO_DIR}/*.proto)
    protobuf_generate(TARGET build_protos LANGUAGE cpp PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/src PROTOS ${SMOKETEST_PROTO} APPEND_PATH ${SMOKETEST_PROTO_DIR})
    protobuf_generate(TARGET build_protos LANGUAGE grpc PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/src PROTOS ${SMOKETEST_PROTO} GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}"  APPEND_PATH ${SMOKETEST_PROTO_DIR})
    target_include_directories(${package_name}
        PRIVATE ${CMAKE_SOURCE_DIR}/test
        PRIVATE ${CMAKE_BINARY_DIR}/src
    )
    target_compile_options(${package_name} PRIVATE -DSMOKE_TEST_SUPPORT)
endif()

add_library(data-relay-grpc-impl INTERFACE)

target_include_directories(data-relay-grpc-impl
        INTERFACE .
        )

target_link_libraries(data-relay-grpc-impl
        INTERFACE ${package_name}
        INTERFACE Boost::boost
        INTERFACE Boost::filesystem
        INTERFACE Boost::thread
        INTERFACE Boost::container
        INTERFACE glog::glog
        INTERFACE grpc++
        INTERFACE grpc
        INTERFACE protobuf
        INTERFACE gpr
        INTERFACE absl_synchronization
)

# tg-grpc-backupd: gRPC remote backup service
file(GLOB TG_GRPC_BACKUPD_SOURCES
        "data_relay_grpc/blob_relay/server_main.cpp"
)
