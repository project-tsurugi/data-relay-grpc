# Generate protobuf and gRPC files
find_program(PROTOBUF_PROTOC protoc)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

file(GLOB CLIENT_SOURCES
        "*.cpp"
)

add_executable(test_client
        ${CLIENT_SOURCES}
        ${CLIENT_PROTO_SRCS}
)

set_target_properties(test_client
        PROPERTIES
                INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
                RUNTIME_OUTPUT_NAME "blob_relay_client"
        )

target_include_directories(test_client
        PRIVATE ${CMAKE_SOURCE_DIR}/include
        PRIVATE ${CMAKE_BINARY_DIR}/src/protos
        PRIVATE ${CMAKE_BINARY_DIR}/src
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
        PRIVATE data-relay-grpc
        PRIVATE data-relay-grpc-impl
        )

set_compile_options(test_client)
