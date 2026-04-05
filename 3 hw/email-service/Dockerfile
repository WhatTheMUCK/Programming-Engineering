# Dockerfile for email-service — native multi-arch build (ARM64 + AMD64)
# Uses FetchContent to build userver from source as part of the project

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Add PostgreSQL 16 repository
RUN apt-get update && apt-get install -y \
    wget \
    gnupg \
    lsb-release \
    && wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - \
    && echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list \
    && apt-get update

# Install all build dependencies in one layer
RUN apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    ccache \
    ninja-build \
    python3 \
    python3-pip \
    python3-venv \
    python3-jinja2 \
    python3-yaml \
    python3-voluptuous \
    # userver core dependencies
    libboost-all-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libev-dev \
    libc-ares-dev \
    libnghttp2-dev \
    libfmt-dev \
    libspdlog-dev \
    libyaml-cpp-dev \
    libhttp-parser-dev \
    zlib1g-dev \
    libbenchmark-dev \
    libgtest-dev \
    libgmock-dev \
    libcrypto++-dev \
    liblz4-dev \
    libzstd-dev \
    libjemalloc-dev \
    # PostgreSQL 16 client and server development libraries
    postgresql-16 \
    postgresql-client-16 \
    postgresql-server-dev-16 \
    libpq-dev \
    # LDAP and Kerberos libraries (required by userver PostgreSQL support)
    libldap2-dev \
    libkrb5-dev \
    # JSON
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Install JWT-CPP library (header-only)
RUN cd /tmp && \
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

# Install Python testing dependencies
RUN pip3 install --no-cache-dir \
    pytest \
    pytest-asyncio \
    PyJWT \
    aiohttp \
    yandex-taxi-testsuite \
    psycopg2-binary

# Set working directory
WORKDIR /app

# Copy source code
COPY . /app

# Build the services (userver is fetched via FetchContent during cmake)
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          .. && \
    make -j2

# Copy binaries to /app
RUN cp /app/build/email-service-user /app/email-service-user && \
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
