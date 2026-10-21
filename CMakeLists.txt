cmake_minimum_required(VERSION 3.10)
project(OrderCache VERSION 1.0 LANGUAGES CXX)

# Set C++ standard to C++17 as required by the assignment
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional Sanitizers for memory and thread-safety verification on Linux/Clang/GCC
option(ENABLE_TSAN "Enable ThreadSanitizer for data race detection" OFF)
option(ENABLE_ASAN "Enable Address and Undefined Behavior Sanitizers" OFF)

# Add compiler flags for optimization in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
  elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -g -O1)
    add_link_options(-fsanitize=thread)
  elseif(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()

# Find Google Test package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add thread support (needed for Google Test)
find_package(Threads REQUIRED)

# Find Protobuf
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Generate C++ sources from protos/order.proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protos/order.proto)

# Source files
set(SOURCES
    OrderCache.cpp
    OrderCacheTest.cpp
    OrderCacheHelpers.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# Add the executable
add_executable(OrderCacheTest ${SOURCES})

# Link against Google Test, Threads, and Protobuf
target_link_libraries(OrderCacheTest
    ${GTEST_BOTH_LIBRARIES}
    Threads::Threads
    ${Protobuf_LIBRARIES}
)

# Enable testing
enable_testing()
add_test(NAME OrderCacheTest COMMAND OrderCacheTest)

# Installation rules (optional)
install(TARGETS OrderCacheTest DESTINATION bin)

# Print configuration summary
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Google Test found: ${GTEST_FOUND}")
message(STATUS "Protobuf found: ${Protobuf_FOUND}")