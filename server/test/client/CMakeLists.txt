# Generate protobuf and gRPC files
find_program(PROTOBUF_PROTOC protoc)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

#
# gRPC proto for test client
#
# Automatically find all .proto files in ${CMAKE_SOURCE_DIR}/../protos/blob_relay/
set(PROTO_DIR ${CMAKE_SOURCE_DIR}/../protos/blob_relay)
file(GLOB CLIENT_PROTO_FILES "${PROTO_DIR}/*.proto")
add_custom_target(client_protos DEPENDS ${CLIENT_PROTO_FILES})

#
# Compile protobuf and grpc files in client_protos target to cpp
#
protobuf_generate(TARGET client_protos LANGUAGE cpp OUT_VAR CLIENT_PROTO_OUT PROTOS ${CLIENT_PROTO_FILES} APPEND_PATH ${PROTO_DIR})
protobuf_generate(TARGET client_protos LANGUAGE grpc OUT_VAR CLIENT_GRPC_OUT PROTOS ${CLIENT_PROTO_FILES} GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}" APPEND_PATH ${PROTO_DIR})

set(CLIENT_PROTO_SRCS)
foreach(SRC_FILE ${CLIENT_PROTO_OUT} ${CLIENT_GRPC_OUT})
  if(${SRC_FILE} MATCHES ".*\.cc")
    list(APPEND CLIENT_PROTO_SRCS ${SRC_FILE})
  endif()
endforeach()

set(SMOKETEST_PROTO_DIR ${CMAKE_SOURCE_DIR}/test/data-relay-grpc/blob_relay/smoke_test/proto)
file(GLOB SMOKETEST_PROTO ${SMOKETEST_PROTO_DIR}/*.proto)
add_custom_target(smoke_test_protos DEPENDS ${PROTO_FILES})
protobuf_generate(TARGET smoke_test_protos LANGUAGE cpp OUT_VAR SMOKE_TEST_OUT PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/test PROTOS ${SMOKETEST_PROTO} APPEND_PATH ${SMOKETEST_PROTO_DIR})
protobuf_generate(TARGET smoke_test_protos LANGUAGE grpc OUT_VAR SMOKE_TEST_GRPC_OUT PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/test PROTOS ${SMOKETEST_PROTO} GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}"  APPEND_PATH ${SMOKETEST_PROTO_DIR})

file(GLOB CLIENT_SOURCES
        "*.cpp"
)

add_executable(test_client
        ${CLIENT_SOURCES}
        ${CLIENT_PROTO_SRCS}
        ${SMOKE_TEST_OUT}
        ${SMOKE_TEST_GRPC_OUT}
)

add_dependencies(test_client
        client_protos
        build_protos
        smoke_test_protos
        )

set_target_properties(test_client
        PROPERTIES
                INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
                RUNTIME_OUTPUT_NAME "blob_relay_client"
        )

target_include_directories(test_client
        PRIVATE ${CMAKE_BINARY_DIR}/src
        PRIVATE ${CMAKE_BINARY_DIR}/test
        PRIVATE ${CMAKE_BINARY_DIR}/test/client
        PRIVATE .
        )

target_link_libraries(test_client
        PRIVATE Boost::boost
        PRIVATE gflags::gflags
        PRIVATE grpc++
        PRIVATE grpc
        PRIVATE gpr
        PRIVATE protobuf
        PRIVATE absl_synchronization
        )

set_compile_options(test_client)
