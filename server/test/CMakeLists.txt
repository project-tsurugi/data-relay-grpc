set(test_target data-relay-grpc-test)

# Generate protobuf and gRPC files
find_program(PROTOBUF_PROTOC protoc)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# For protos used in test only
file(GLOB_RECURSE PROTO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.proto")
foreach(FILE ${PROTO_FILES})
  if(${FILE} MATCHES "data-relay-grpc/*")
    list(APPEND TEST_PROTO_FILES ${FILE})
  endif()
endforeach()
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


# Add generated files to sources
list(APPEND SOURCES ${TEST_PROTO_SRCS})

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
        test_protos
        )

target_include_directories(${test_target}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_BINARY_DIR}/src
        PRIVATE ${CMAKE_BINARY_DIR}/test
        PRIVATE ${CMAKE_BINARY_DIR}/test/data-relay-grpc/grpc/proto
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

add_subdirectory(client)
