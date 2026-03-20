# Dockerfile using prebuilt userver image with SQLite
# Optimized single-stage build

FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg-dev:latest

# Install all dependencies in one layer
RUN apt-get update && \
    apt-get install -y \
    libsqlite3-dev \
    sqlite3 \
    wget \
    libssl-dev \
    git \
    nlohmann-json3-dev \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/* && \
    # Install Python testing dependencies
    pip3 install --no-cache-dir \
    pytest \
    pytest-asyncio \
    PyJWT \
    aiohttp && \
    # Install JWT-CPP library (header-only)
    cd /tmp && \
    git clone --depth 1 --branch v0.7.0 https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && \
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DJWT_BUILD_EXAMPLES=OFF \
          -DJWT_BUILD_TESTS=OFF \
          -DJWT_INSTALL=ON \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DDEFAULT_NLOHMANN_JSON_BACKEND=OFF \
          .. && \
    make install && \
    ldconfig && \
    rm -rf /tmp/jwt-cpp

# Set working directory
WORKDIR /app

# Copy source code
COPY . /app

# Build the services
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DUSERVER_FEATURE_POSTGRESQL=OFF \
          -DUSERVER_FEATURE_TESTSUITE=OFF \
          -DUSERVER_FEATURE_FORMAT=OFF \
          .. && \
    make -j2

# Create data directory and copy binaries to /app
RUN mkdir -p /app/data && \
    cp /app/build/email-service-user /app/email-service-user && \
    cp /app/build/email-service-folder /app/email-service-folder && \
    cp /app/build/email-service-message /app/email-service-message && \
    chmod +x /app/email-service-user \
    && chmod +x /app/email-service-folder \
    && chmod +x /app/email-service-message

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD wget --quiet --tries=1 --spider http://localhost:8080/ping || exit 1

# Default command (will be overridden in docker-compose)
CMD ["/bin/bash"]
