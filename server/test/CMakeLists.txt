set(test_target data-relay-grpc-test)

# Generate protobuf and gRPC files
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# For protos used in test only
file(GLOB TEST_PROTO_FILES
        ${CMAKE_SOURCE_DIR}/test/data_relay_grpc/grpc/proto/*.proto
)
add_custom_target(test_protos DEPENDS ${TEST_PROTO_FILES})

#
# Compile protobuf and grpc files in test_protos target to cpp
#
protobuf_generate(TARGET test_protos LANGUAGE cpp OUT_VAR TEST_PROTO_OUT PROTOS ${TEST_PROTO_FILES})
protobuf_generate(TARGET test_protos LANGUAGE grpc OUT_VAR TEST_GRPC_OUT PROTOS ${TEST_PROTO_FILES} GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}")

set(TEST_PROTO_SRCS)
foreach(SRC_FILE ${TEST_PROTO_OUT} ${TEST_GRPC_OUT})
  if(${SRC_FILE} MATCHES ".*\.cc")
    list(APPEND TEST_PROTO_SRCS ${SRC_FILE})
  endif()
endforeach()

set_source_files_properties(
        ${TEST_PROTO_SRCS}
        PROPERTIES
        GENERATED TRUE
        COMPILE_FLAGS -Wno-unused-parameter
)

add_executable(${test_target}
        main.cpp
        ${TEST_PROTO_SRCS}
        )

add_dependencies(${test_target}
        build_protos
        test_protos
        )

target_include_directories(${test_target}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_BINARY_DIR}/src
        PRIVATE ${CMAKE_BINARY_DIR}/test
        PRIVATE ${CMAKE_BINARY_DIR}/test/data_relay_grpc/grpc/proto
        )

target_link_libraries(${test_target}
        PRIVATE data-relay-grpc
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
        "data_relay_grpc/blob_relay/*.cpp"
        "data_relay_grpc/common/*.cpp"
        "data_relay_grpc/grpc/*.cpp"
        "data_relay_grpc/grpc/service/*.cpp"
        )

foreach(file ${SRCS})
    add_test_executable(${file})
endforeach()

if(SMOKE_TEST_SUPPORT)
    add_subdirectory(client)
endif()
