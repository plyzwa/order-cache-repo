# Makefile for OrderCache Project

BUILD_DIR = build
CMAKE = cmake
CTEST = ctest

.PHONY: all configure release debug build test clean help

all: release build test

# Configure CMake with Release mode (Default)
release:
	$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Release

# Configure CMake with Debug mode
debug:
	$(CMAKE) -B $(BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Debug

# Compile binaries
build:
	$(CMAKE) --build $(BUILD_DIR)

# Run Google Test suite
test: build
	./$(BUILD_DIR)/OrderCacheTest

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Available targets:"
	@echo "  make release    - Configure CMake in Release mode (-O3)"
	@echo "  make debug      - Configure CMake in Debug mode (-g)"
	@echo "  make build      - Compile the test executable"
	@echo "  make test       - Build and run the test suite"
	@echo "  make clean      - Remove build directory"