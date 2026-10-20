# Order Cache Testing Guide

## Table of Contents
- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Environment Setup](#environment-setup)
  - [Windows](#windows)
  - [Linux](#linux)
  - [macOS](#macos)
- [Building the Tests](#building-the-tests)
  - [Using Command Line](#using-command-line)
  - [Using CMake](#using-cmake)
- [Running the Tests](#running-the-tests)
- [Understanding Test Results](#understanding-test-results)
- [Test Expectations](#test-expectations)
- [Troubleshooting](#troubleshooting)

## Overview

This document provides instructions for setting up, building, and running the unit tests for your OrderCache implementation. You are required to write your own test suite in `OrderCacheTest.cpp` using the Google Test framework.

## Prerequisites

To compile and run the tests, you need:

- A C++ compiler with C++17 support (GCC 7+, Clang 5+, or MSVC 19.14+)
- Google Test library (version 1.10.0 or later recommended)
- CMake (optional, version 3.10 or later)
- Make or equivalent build system

## Environment Setup

### Windows

#### Using Visual Studio

1. Install Visual Studio 2019 or later with C++ development workload
2. Use the built-in Google Test support:
   - In Solution Explorer, right-click on your project
   - Select "Manage NuGet Packages"
   - Search for "Google Test" and install it
   - Detailed instructions: [Google Test in Visual Studio](https://learn.microsoft.com/en-us/visualstudio/test/how-to-use-google-test-for-cpp?view=vs-2022)

#### Using MinGW/MSYS2

1. Install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/)
2. Open MSYS2 terminal and run:
   ```
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-gtest
   ```

### Linux

#### Ubuntu/Debian

```bash
# Install compiler and dependencies
sudo apt-get update
sudo apt-get install -y build-essential cmake

# Install Google Test
sudo apt-get install -y libgtest-dev
cd /usr/src/googletest/
sudo cmake .
sudo cmake --build . --target install
```

#### Fedora/RHEL/CentOS

```bash
# Install compiler and dependencies
sudo dnf install -y gcc-c++ cmake

# Install Google Test
sudo dnf install -y gtest-devel
```

### macOS

```bash
# Using Homebrew
brew install cmake
brew install googletest
```

## Building the Tests

### Using Command Line

#### Linux/macOS

```bash
# Basic compilation
g++ --std=c++17 -O2 OrderCacheTest.cpp OrderCache.cpp -o OrderCacheTest -lgtest -lgtest_main -pthread

# With debugging information
g++ --std=c++17 -g OrderCacheTest.cpp OrderCache.cpp -o OrderCacheTest_debug -lgtest -lgtest_main -pthread

# With performance optimizations (for final testing)
g++ --std=c++17 -O3 -march=native OrderCacheTest.cpp OrderCache.cpp -o OrderCacheTest_optimized -lgtest -lgtest_main -pthread
```

#### Windows (MinGW)

```bash
g++ --std=c++17 -O2 OrderCacheTest.cpp OrderCache.cpp -o OrderCacheTest.exe -lgtest -lgtest_main -pthread
```

### Using CMake

A `CMakeLists.txt` file is provided that supports multiple build types: Default, Debug, and Release.

Build instructions:

```bash
# Create a build directory
mkdir build
cd build

# Configure and build different versions:

# 1. Debug build (with debugging information)
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# 2. Release build (with performance optimizations)
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# 3. Default build
cmake ..
cmake --build .
```

The different build types are ideal for different phases of development:
- **Debug build**: Use during development for easier debugging
- **Release build**: Use for performance testing
- **Default build**: Basic compilation without specific optimizations

## Running the Tests

Execute the test binary:

```bash
# Run all tests
./OrderCacheTest

# Run with verbose output
./OrderCacheTest --gtest_verbose

# Run a subset of tests using a filter
./OrderCacheTest --gtest_filter=YourFilterHere*
```

## Understanding Test Results

The test output will display:

- Results for each test case (PASSED/FAILED)
- For failed tests, the assertion that caused the failure and its location
- A summary of passed and failed tests

Example output:
```
[==========] Running 20 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 20 tests from OrderCacheTest
[ RUN      ] OrderCacheTest.SomeTest
[       OK ] OrderCacheTest.SomeTest (0 ms)
...
[----------] 20 tests from OrderCacheTest (50 ms total)
[==========] 20 tests from 1 test suite ran. (50 ms total)
[  PASSED  ] 20 tests.
```

## Test Expectations

Write a test suite that gives you confidence your implementation is correct and robust. Your tests should cover the functionality described in the README, including the order matching examples provided there. Think about what could go wrong and how the cache should behave in those situations.

The quality of your test suite is part of the evaluation. We are looking for evidence of thoughtful test design — not just quantity of tests.

## Troubleshooting

### Common Issues

1. **Google Test not found**
   - Ensure Google Test is installed correctly
   - Check library paths with `find /usr -name "libgtest*.a"` on Linux
   - Use the `-I` and `-L` flags to specify include and library paths

2. **Compilation errors**
   - Ensure you have C++17 support: `g++ --version`
   - Check for missing header files or dependencies
   - Make sure OrderCache.h and OrderCache.cpp are in the same directory

3. **Test failures**
   - Check output carefully for specific test failure reasons
   - Look for assertion messages that identify the exact failure point
   - Fix one failure at a time, starting with basic operations

4. **Performance issues**
   - Profile your code to identify bottlenecks
   - Consider more efficient data structures
   - Look for unnecessary copying or redundant calculations

### Library Installation Issues

If you encounter issues with Google Test installation:

#### Linux (Alternative Method)
```bash
# Clone Google Test repository
git clone https://github.com/google/googletest.git
cd googletest
mkdir build && cd build
cmake ..
make
sudo make install
```

#### Windows (Alternative Method)
```bash
# Clone Google Test repository
git clone https://github.com/google/googletest.git
cd googletest
mkdir build && cd build
cmake ..
cmake --build . --config Release
```
