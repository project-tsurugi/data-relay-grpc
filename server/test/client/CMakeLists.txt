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


# For protos used by client only
file(GLOB_RECURSE PROTO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.proto")
foreach(FILE ${PROTO_FILES})
  if(${FILE} MATCHES "tateyama*")
    list(APPEND TATEYAMA_PROTO_FILES ${FILE})
  endif()
endforeach()
add_custom_target(tateyama_protos DEPENDS ${TATEYAMA_PROTO_FILES})
#
# Compile protobuf files in tateyama_protos target to cpp
#
# Generate protobuf files
protobuf_generate(TARGET tateyama_protos LANGUAGE cpp OUT_VAR TATEYAMA_PROTO_OUT PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/test/client PROTOS ${TATEYAMA_PROTO_FILES})

set(TATEYAMA_PROTO_SRCS)
foreach(FILE ${TATEYAMA_PROTO_OUT})
  if(${FILE} MATCHES ".*\.cc")
    list(APPEND TATEYAMA_PROTO_SRCS ${FILE})
  endif()
endforeach()

file(GLOB CLIENT_SOURCES
        "*.cpp"
        "tateyama/utils/*.cpp"
)

add_executable(test_client
        ${CLIENT_SOURCES}
        ${CLIENT_PROTO_SRCS}
        ${TATEYAMA_PROTO_SRCS}
)

add_dependencies(test_client
        client_protos
        tateyama_protos
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
