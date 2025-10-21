set(test_target data-relay-grpc-test)

# Generate protobuf and gRPC files
find_program(PROTOBUF_PROTOC protoc)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# Automatically find all .proto files in ${CMAKE_SOURCE_DIR}/../protos/blob_relay
file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/../protos/blob_relay/*.proto")
file(GLOB TEST_PROTO_FILES "data-relay-grpc/grpc/proto/*.proto")
set(TEST_PROTO_SRCS)
set(TEST_PROTO_HDRS)
set(TEST_GRPC_SRCS)
set(TEST_GRPC_HDRS)

foreach(PROTO_FILE ${TEST_PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(TEST_PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    set(TEST_PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
    set(TEST_GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(TEST_GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")
    list(APPEND TEST_PROTO_SRCS ${TEST_PROTO_SRC})
    list(APPEND TEST_PROTO_HDRS ${TEST_PROTO_HDR})
    list(APPEND TEST_GRPC_SRCS ${TEST_GRPC_SRC})
    list(APPEND TEST_GRPC_HDRS ${TEST_GRPC_HDR})

    add_custom_command(
        OUTPUT ${TEST_PROTO_SRC} ${TEST_PROTO_HDR} ${TEST_GRPC_SRC} ${TEST_GRPC_HDR}
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
             --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
             -I "${CMAKE_CURRENT_SOURCE_DIR}/data-relay-grpc/grpc/proto"
             --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
             "${PROTO_FILE}"
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf and gRPC files for ${PROTO_FILE}"
    )
endforeach()

# Add generated files to sources
list(APPEND SOURCES ${TEST_PROTO_SRCS} ${TEST_GRPC_SRCS})

add_custom_target(build_test_protos DEPENDS ${TEST_PROTO_SRCS} ${TEST_GRPC_SRCS})

set_source_files_properties(
        ${TEST_PROTO_SRCS} ${TEST_GRPC_SRCS}
        PROPERTIES
        GENERATED TRUE
        COMPILE_FLAGS -Wno-unused-parameter
)

add_executable(${test_target}
        main.cpp
        ${TEST_PROTO_SRCS}
        ${TEST_GRPC_SRCS}
        )

add_dependencies(${test_target}
        build_test_protos
        )

target_include_directories(${test_target}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_BINARY_DIR}/src
        PRIVATE ${CMAKE_BINARY_DIR}/test
        )

target_link_libraries(${test_target}
        PRIVATE data-relay-grpc-impl
        PUBLIC gtest
        PRIVATE limestone
        )

function (add_test_executable source_file)
    get_filename_component(test_name "${source_file}" NAME_WE)
    target_sources(${test_target}
            PRIVATE ${source_file}
            )
    add_test(
            NAME ${test_name}
            COMMAND ${test_target} --gtest_filter=${test_name}.* --gtest_output=xml:${test_name}_gtest_result.xml
    )
endfunction (add_test_executable)

file(GLOB SRCS
        "data-relay-grpc/blob_relay/*.cpp"
        "data-relay-grpc/grpc/*.cpp"
        "data-relay-grpc/grpc/service/*.cpp"
        )

foreach(file ${SRCS})
    add_test_executable(${file})
endforeach()
