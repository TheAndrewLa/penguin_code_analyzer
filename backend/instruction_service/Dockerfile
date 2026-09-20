# syntax=docker/dockerfile:1

FROM ubuntu:24.04 AS deps

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    curl \
    libssl-dev \
    zlib1g-dev \
    libjsoncpp-dev \
    uuid-dev \
    libbrotli-dev \
    llvm-18-dev \
    llvm-18-tools \
    && rm -rf /var/lib/apt/lists/*

FROM deps AS drogon-builder

ARG DROGON_VERSION=v1.9.7

RUN git clone --depth 1 --branch ${DROGON_VERSION} \
    https://github.com/drogonframework/drogon.git /opt/drogon && \
    cd /opt/drogon && \
    git submodule update --init --recursive && \
    cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_CTL=OFF \
    -DBUILD_ORM=OFF \
    -DBUILD_DROGON_SHARED=OFF \
    -DBUILD_TRANTOR_SHARED=OFF && \
    cmake --build build -j"$(nproc)" && \
    cmake --install build

FROM deps AS build

COPY --from=drogon-builder /usr/local /usr/local

WORKDIR /app

COPY . .

ENV LLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm

RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/usr/lib/llvm-18;/usr/local" && \
    cmake --build build -j"$(nproc)"

FROM build AS test

RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    jq \
    && rm -rf /var/lib/apt/lists/*

RUN ctest --test-dir build --output-on-failure

FROM build AS runtime

EXPOSE 8080

ENTRYPOINT ["/app/build/instruction_service"]
CMD ["--port=8080"]
