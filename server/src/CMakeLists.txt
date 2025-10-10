file(GLOB SOURCES
        "data-relay-grpc/blob_relay/*.cpp"
)

# Generate protobuf and gRPC files
find_program(PROTOBUF_PROTOC protoc)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

# Automatically find all .proto files in ${CMAKE_SOURCE_DIR}/proto/
file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/../protos/blob_relay/*.proto")
set(PROTO_SRCS)
set(PROTO_HDRS)
set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h")
    set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc")
    set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h")
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
    list(APPEND GRPC_SRCS ${GRPC_SRC})
    list(APPEND GRPC_HDRS ${GRPC_HDR})

    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR} ${GRPC_SRC} ${GRPC_HDR}
        COMMAND ${PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
             --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
             -I "${CMAKE_SOURCE_DIR}/../protos/blob_relay"
             --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN_EXECUTABLE}"
             "${PROTO_FILE}"
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating protobuf and gRPC files for ${PROTO_FILE}"
    )
endforeach()

# Add generated files to sources
list(APPEND SOURCES ${PROTO_SRCS} ${GRPC_SRCS})

add_library(${package_name}
        ${SOURCES}
)

set_target_properties(${package_name}
        PROPERTIES
                INSTALL_RPATH "\$ORIGIN"
                OUTPUT_NAME ${export_name}
)
target_include_directories(${package_name}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}   # internal sources
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}   # for generated pb.h, not installed
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
        PRIVATE limestone
)

if (ENABLE_ALTIMETER)
    target_link_libraries(${package_name}
        PRIVATE altimeter
    )
endif()

set_compile_options(${package_name})

install_custom(${package_name} ${export_name})

# for tests
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
        "data-relay-grpc/blob_relay/server_main.cpp"
)
