cmake_minimum_required(VERSION 3.10)
project(InstantMessagingService)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adding sanitizers and other flags

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=maybe-uninitialized")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=maybe-uninitialized -fsanitize=address")
#for clang to know about cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#gtest installation
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)  
FetchContent_MakeAvailable(googletest)
#Adding tests
enable_testing()

#include headers here
set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/includes)
include(GoogleTest)
# Protobufs
find_package(protobuf CONFIG REQUIRED)
set(PROTOS ${CMAKE_SOURCE_DIR}/protos)
#generating protos
add_library(proto-objects OBJECT ${PROTOS}/main.proto)
target_link_libraries(proto-objects PUBLIC protobuf::libprotobuf)
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
protobuf_generate(
    TARGET proto-objects
		IMPORT_DIRS ${PROTOS}
) 
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
add_subdirectory(src)

