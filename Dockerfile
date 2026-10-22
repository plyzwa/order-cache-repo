# ==============================================================================
# Stage 1: Build Environment (Toolchain & Dependencies)
# ==============================================================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build toolchain, generators, and package libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    protobuf-compiler \
    libprotobuf-dev \
    libgtest-dev \
    libbenchmark-dev \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy complete project source
COPY . .

# Configure and compile in optimized Release mode (-O3, -DNDEBUG)
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --config Release -j$(nproc)

# Optional: Run test suite during container build to prevent broken artifacts
RUN ./build/OrderCacheTest

# ==============================================================================
# Stage 2: Minimal Runtime Environment
# ==============================================================================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install only the runtime libraries needed by the compiled binaries
RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf23 \
    libstdc++6 \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled binaries from the builder stage
COPY --from=builder /workspace/build/OrderCacheTest /app/OrderCacheTest
COPY --from=builder /workspace/build/OrderCacheBenchmark /app/OrderCacheBenchmark

# Default entrypoint runs the test suite
ENTRYPOINT ["/app/OrderCacheTest"]