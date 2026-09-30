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
    GRAPH_SERVICE_URL=http://graph_service:9000 \
    GCC_CC=/usr/bin/gcc \
    CLANG_CC=/usr/bin/clang-19

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    build-essential \
    clang-19 \
    && ln -s /usr/bin/clang-19 /usr/local/bin/clang \
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
