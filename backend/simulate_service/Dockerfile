# STAGE 1: Build
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    pkg-config \
    llvm-19-dev \
    libclang-19-dev \
    libzstd-dev \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libc-ares-dev \
    libbrotli-dev \
    && rm -rf /var/lib/apt/lists/*

ARG DROGON_VERSION=v1.9.8
WORKDIR /opt
RUN git clone --depth 1 --branch ${DROGON_VERSION} \
    https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git submodule update --init --recursive && \
    cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_CTL=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_DOC=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build build -j"$(nproc)" && \
    cmake --install build && \
    ldconfig

WORKDIR /app
COPY CMakeLists.txt ./
COPY src ./src
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build build -j"$(nproc)"

# STAGE 2: Runtime
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3t64 \
    libjsoncpp25 \
    libuuid1 \
    zlib1g \
    libc-ares2 \
    libbrotli1 \
    libzstd1 \
    libllvm19 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libdrogon*.so* /usr/local/lib/
RUN ldconfig

COPY --from=builder /app/build/service /usr/local/bin/service

RUN useradd --system --create-home --uid 10001 appuser

WORKDIR /app

# Create Drogon's upload tree: /app/upload/tmp/{00..FF}
RUN mkdir -p /app/uploads/tmp && \
    for i in $(seq 0 255); do \
        hex=$(printf '%02X' "$i"); \
        mkdir -p "/app/uploads/tmp/$hex"; \
    done && \
    chown -R appuser:appuser /app/uploads

USER appuser

EXPOSE 8080

ENTRYPOINT ["/usr/local/bin/service"]
CMD ["--port", "8080"]
