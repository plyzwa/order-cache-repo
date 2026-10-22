cmake_minimum_required(VERSION 3.14)

# Default build type to Release prior to any target/dependency declarations
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose build type: Debug or Release." FORCE)
endif()

# Allow subprojects (hiredis, redis-plus-plus) with legacy cmake_minimum_required (< 3.10) to configure on modern CMake (3.30+)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for subprojects" FORCE)

project(OrderCache VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional Sanitizers for memory and thread-safety verification on Linux/Clang/GCC
option(ENABLE_TSAN "Enable ThreadSanitizer for data race detection" OFF)
option(ENABLE_ASAN "Enable Address and Undefined Behavior Sanitizers" OFF)

# Compiler flags for optimization and disabling assertions in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
  elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
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

# =============================== SYSTEM PACKAGES ===============================
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
find_package(Protobuf REQUIRED)
find_package(benchmark REQUIRED)

# Generate C++ sources from protos/order.proto
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS protos/order.proto)

# =============================== FETCH CONTENT ===============================
include(FetchContent)

# 1. Google FlatBuffers (Zero-Copy Serialization)
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG        v24.3.25
)
FetchContent_MakeAvailable(flatbuffers)

# FlatBuffers schema code generation rule
set(FBS_SCHEMA ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffer/order.fbs)
set(FBS_HEADER ${CMAKE_CURRENT_BINARY_DIR}/order_generated.h)

add_custom_command(
    OUTPUT ${FBS_HEADER}
    COMMAND $<TARGET_FILE:flatc> --cpp -o ${CMAKE_CURRENT_BINARY_DIR} ${FBS_SCHEMA}
    DEPENDS ${FBS_SCHEMA} flatc
    COMMENT "Compiling FlatBuffers schema: ${FBS_SCHEMA}"
)
add_custom_target(generate_fbs DEPENDS ${FBS_HEADER})

# 2. Hiredis (Official C Redis driver)
set(DISABLE_TESTS ON CACHE BOOL "" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    hiredis
    GIT_REPOSITORY https://github.com/redis/hiredis.git
    GIT_TAG        v1.2.0
)

FetchContent_GetProperties(hiredis)
if(NOT hiredis_POPULATED)
    FetchContent_Populate(hiredis)
    file(MAKE_DIRECTORY ${hiredis_SOURCE_DIR}/include/hiredis)
    file(COPY
        ${hiredis_SOURCE_DIR}/hiredis.h
        ${hiredis_SOURCE_DIR}/read.h
        ${hiredis_SOURCE_DIR}/sds.h
        ${hiredis_SOURCE_DIR}/async.h
        ${hiredis_SOURCE_DIR}/alloc.h
        ${hiredis_SOURCE_DIR}/sockcompat.h
        DESTINATION ${hiredis_SOURCE_DIR}/include/hiredis
    )
    add_subdirectory(${hiredis_SOURCE_DIR} ${hiredis_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# 3. Redis-plus-plus (C++17 RAII Redis client)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_CXX_STANDARD 17 CACHE STRING "" FORCE)

set(HIREDIS_HEADER ${hiredis_SOURCE_DIR}/include CACHE PATH "Path to hiredis headers" FORCE)
set(HIREDIS_LIB hiredis CACHE FILEPATH "Hiredis library target" FORCE)
set(TEST_HIREDIS_LIB hiredis CACHE FILEPATH "Hiredis test library target" FORCE)

FetchContent_Declare(
    redis_plus_plus
    GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
    GIT_TAG        1.3.12
)
FetchContent_GetProperties(redis_plus_plus)
if(NOT redis_plus_plus_POPULATED)
    FetchContent_Populate(redis_plus_plus)
    add_subdirectory(${redis_plus_plus_SOURCE_DIR} ${redis_plus_plus_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(TARGET redis++_static AND NOT TARGET redis++)
    add_library(redis++ ALIAS redis++_static)
endif()

# =============================== TEST EXECUTABLE ===============================
set(SOURCES
    OrderCache.cpp
    OrderCacheTest.cpp
    OrderCacheHelpers.cpp
    RedisClient.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${FBS_HEADER}
)

add_executable(OrderCacheTest ${SOURCES})

# Ensure schema headers are generated prior to compiling C++ translation units
add_dependencies(OrderCacheTest generate_fbs)

target_include_directories(OrderCacheTest PRIVATE
    ${GTEST_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${hiredis_SOURCE_DIR}/include
)

target_link_libraries(OrderCacheTest PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    Threads::Threads
    ${Protobuf_LIBRARIES}
    flatbuffers
    redis++
    hiredis
)

if(WIN32)
    target_link_libraries(OrderCacheTest PRIVATE ws2_32 crypt32)
endif()

# =============================== BENCHMARK EXECUTABLE ===============================
set(BENCHMARK_SOURCES
    OrderCacheBenchmark.cpp
    OrderCacheBenchmark.h
    OrderCache.cpp
    OrderCacheHelpers.cpp
    RedisClient.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${FBS_HEADER}
)

add_executable(OrderCacheBenchmark ${BENCHMARK_SOURCES})

add_dependencies(OrderCacheBenchmark generate_fbs)

target_include_directories(OrderCacheBenchmark PRIVATE
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${hiredis_SOURCE_DIR}/include
)

target_link_libraries(OrderCacheBenchmark PRIVATE
    benchmark::benchmark
    Threads::Threads
    ${Protobuf_LIBRARIES}
    flatbuffers
    redis++
    hiredis
)

if(WIN32)
    target_link_libraries(OrderCacheBenchmark PRIVATE ws2_32 crypt32)
endif()

# =============================== TESTING & INSTALL ===============================
enable_testing()
add_test(NAME OrderCacheTest COMMAND OrderCacheTest)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Google Test found: ${GTEST_FOUND}")
message(STATUS "Protobuf found: ${Protobuf_FOUND}")
message(STATUS "Google Benchmark found: ${benchmark_FOUND}")