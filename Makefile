# Makefile for OrderCache Project

BUILD_DIR = build
CMAKE = cmake
CTEST = ctest

.PHONY: all configure release debug build test benchmark clean help

all: release build test benchmark

# Configure CMake with Release mode (Default)
release:
	$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Release

# Configure CMake with Debug mode
debug:
	$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Debug

# Compile all binaries with Release configuration
build:
	$(CMAKE) --build $(BUILD_DIR) --config Release

# Run Google Test suite
test: build
	./$(BUILD_DIR)/OrderCacheTest

# Run Google Benchmark executable
benchmark: build
	./$(BUILD_DIR)/OrderCacheBenchmark

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Available targets:"
	@echo "  make release    - Configure CMake in Release mode (-O3, -DNDEBUG)"
	@echo "  make debug      - Configure CMake in Debug mode (-g)"
	@echo "  make build      - Compile test and benchmark executables"
	@echo "  make test       - Build and run the unit test suite"
	@echo "  make benchmark  - Build and execute the benchmark suite"
	@echo "  make clean      - Remove build directory"