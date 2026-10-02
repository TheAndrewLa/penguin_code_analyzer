# ---------- STAGE 1: BUILD ----------
FROM node:22-bookworm-slim AS build

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci

COPY index.html tsconfig.json vite.config.ts postcss.config.js tailwind.config.js ./
COPY public ./public
COPY src ./src
RUN npm run build

# ---------- STAGE 2: RUNTIME ----------
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive \
    BFF_PORT=3000 \
    INSTRUCTION_SERVICE_URL=http://instruction_service:8080 \
    SIMULATE_SERVICE_URL=http://simulate_service:8090 \
    GRAPH_SERVICE_URL=http://graph_service:9000

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    gnupg \
    software-properties-common \
    libc6-dev \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
    | gpg --dearmor -o /etc/apt/trusted.gpg.d/llvm.gpg \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-19 main" > /etc/apt/sources.list.d/llvm-19.list \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main" > /etc/apt/sources.list.d/llvm-20.list \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-21 main" > /etc/apt/sources.list.d/llvm-21.list \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-22 main" > /etc/apt/sources.list.d/llvm-22.list \
    && echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-23 main" > /etc/apt/sources.list.d/llvm-23.list \
    && apt-get update && apt-get install -y --no-install-recommends \
    gcc-11 \
    gcc-12 \
    gcc-13 \
    gcc-14 \
    gcc-15 \
    clang-19 \
    clang-20 \
    clang-21 \
    clang-22 \
    clang-23 \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://deb.nodesource.com/setup_22.x --no-check-certificate \
    && bash setup_22.x \
    && apt-get install -y --no-install-recommends nodejs \
    && rm -rf /var/lib/apt/lists/* \
    && rm setup_22.x

COPY --from=build /app/dist /app/dist
COPY server /app/server

RUN useradd --system --create-home --uid 10001 nodeuser

WORKDIR /app
USER nodeuser
EXPOSE 3000

CMD ["node", "server/index.mjs"]
