cmake_minimum_required(VERSION 3.5)
project(livekit)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wunused-but-set-parameter -Wformat-security -Wformat-security -Wstrict-null-sentinel -Werror")


set(FFI_PROTO_PATH client-sdk-rust/livekit-ffi/protocol)
set(FFI_PROTO_FILES
    ${FFI_PROTO_PATH}/handle.proto
    ${FFI_PROTO_PATH}/ffi.proto
    ${FFI_PROTO_PATH}/participant.proto
    ${FFI_PROTO_PATH}/room.proto
    ${FFI_PROTO_PATH}/track.proto
    ${FFI_PROTO_PATH}/video_frame.proto
    ${FFI_PROTO_PATH}/audio_frame.proto
    ${FFI_PROTO_PATH}/e2ee.proto
    ${FFI_PROTO_PATH}/stats.proto
)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_BINARY_DIR})

find_package(protobuf REQUIRED CONFIG)

# livekit-proto
add_library(livekit_proto OBJECT ${FFI_PROTO_FILES})
target_link_libraries(livekit_proto PUBLIC protobuf::libprotobuf)
target_include_directories(livekit_proto PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")
target_include_directories(livekit_proto PUBLIC ${Protobuf_INCLUDE_DIRS})

protobuf_generate(
    TARGET livekit_proto
    PROTOS ${FFI_PROTO_FILES} 
    APPEND_PATH ${FFI_PROTO_PATH}
    IMPORT_DIRS ${FFI_PROTO_PATH}
    PROTOC_OUT_DIR ${PROTO_BINARY_DIR}
)

# livekit
add_library(livekit
    include/livekit/ffi_client.h
    include/livekit/livekit.h
    include/livekit/video_frame.h
    include/livekit/track.h
    include/livekit/video_source.h
    include/livekit/participant.h
    include/livekit/room.h
    src/ffi_client.cpp
    src/video_frame.cpp
    src/track.cpp
    src/video_source.cpp
    src/participant.cpp
    src/room.cpp
)

include(FetchContent)

FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5.0  # Specify a version tag
)
FetchContent_MakeAvailable(Corrosion)

# Add Rust static library and generate header

# Import the Rust crate
corrosion_import_crate(
    MANIFEST_PATH client-sdk-rust/Cargo.toml
    CRATE_TYPES staticlib
    CRATES livekit-ffi
)

# Generate C bindings using cbindgen
corrosion_experimental_cbindgen(
    TARGET livekit_ffi  # This should match the name of your imported Rust target
    HEADER_NAME "livekit_ffi.h"
    CARGO_PACKAGE livekit-ffi
    FLAGS --config ${CMAKE_CURRENT_SOURCE_DIR}/client-sdk-rust/livekit-ffi/cbindgen.toml
)

# Update livekit target
target_link_libraries(livekit PUBLIC livekit_proto livekit_ffi)

# Include the auto-generated files from livekit-ffi (C headers)
# target_include_directories(livekit PUBLIC client-sdk-rust/livekit-ffi/include/)
target_include_directories(livekit PUBLIC include/)


# Examples
add_subdirectory(examples)


add_custom_target(print_livekit_ffi_sources
    COMMAND ${CMAKE_COMMAND} -E echo "livekit_ffi INTERFACE_SOURCES: $<TARGET_PROPERTY:livekit_ffi,INTERFACE_SOURCES>"
    COMMAND ${CMAKE_COMMAND} -E echo "livekit_ffi INTERFACE_HEADER_SETS: $<TARGET_PROPERTY:livekit_ffi,INTERFACE_HEADER_SETS>"
)

add_custom_target(print_livekit_ffi_header_set
    COMMAND ${CMAKE_COMMAND} -E echo "livekit_ffi INTERFACE_HEADER_SET_HEADERS: $<TARGET_PROPERTY:livekit_ffi,INTERFACE_HEADER_SET_HEADERS>"
    COMMAND ${CMAKE_COMMAND} -E echo "livekit_ffi HEADER_SET_HEADERS_BASE_DIRS: $<TARGET_PROPERTY:livekit_ffi,HEADER_SET_HEADERS_BASE_DIRS>"
)
