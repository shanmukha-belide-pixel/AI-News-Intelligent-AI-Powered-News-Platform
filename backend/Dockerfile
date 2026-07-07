# ===========================================================================
# AI News Platform — Multi-stage Dockerfile (C++20 / Drogon)
# ===========================================================================

# ─── Stage 1: Builder ────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS builder

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Kolkata

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    libboost-all-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libpq-dev \
    libhiredis-dev \
    libcurl4-openssl-dev \
    librdkafka-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# ── Install Drogon from source (C++20 compatible) ────────────────────────────
RUN git clone --depth 1 --recurse-submodules \
    https://github.com/drogonframework/drogon /tmp/drogon && \
    cmake -S /tmp/drogon -B /tmp/drogon/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_CTL=OFF \
      -DBUILD_EXAMPLES=OFF \
      -DBUILD_TESTING=OFF && \
    cmake --build /tmp/drogon/build -j$(nproc) && \
    cmake --install /tmp/drogon/build && \
    rm -rf /tmp/drogon

# ── Install jwt-cpp (header-only) ────────────────────────────────────────────
RUN git clone --depth 1 https://github.com/Thalhammer/jwt-cpp /tmp/jwt-cpp && \
    cp -r /tmp/jwt-cpp/include/jwt-cpp /usr/local/include/ && \
    rm -rf /tmp/jwt-cpp

# ── Install nlohmann/json (header-only) ──────────────────────────────────────
RUN wget -q https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp \
    -O /usr/local/include/nlohmann/json.hpp

# ── Install spdlog ────────────────────────────────────────────────────────────
RUN git clone --depth 1 https://github.com/gabime/spdlog /tmp/spdlog && \
    cmake -S /tmp/spdlog -B /tmp/spdlog/build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /tmp/spdlog/build -j$(nproc) && \
    cmake --install /tmp/spdlog/build && \
    rm -rf /tmp/spdlog

# ── Install redis-plus-plus ────────────────────────────────────────────────────
RUN git clone --depth 1 https://github.com/sewenew/redis-plus-plus /tmp/redis-plus-plus && \
    cmake -S /tmp/redis-plus-plus -B /tmp/redis-plus-plus/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DREDIS_PLUS_PLUS_BUILD_TEST=OFF && \
    cmake --build /tmp/redis-plus-plus/build -j$(nproc) && \
    cmake --install /tmp/redis-plus-plus/build && \
    rm -rf /tmp/redis-plus-plus

# ── Copy source and build ────────────────────────────────────────────────────
WORKDIR /src
COPY . .

RUN cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_STANDARD=20 && \
    cmake --build build -j$(nproc)

# ─── Stage 2: Runtime ────────────────────────────────────────────────────────
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Kolkata

RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.83.0 \
    libpq5 \
    libhiredis0.14 \
    libcurl4 \
    librdkafka++1 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy compiled binary and configs
COPY --from=builder /src/build/news_server /usr/local/bin/news_server
COPY --from=builder /usr/local/lib/libdrogon.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libredis++.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libspdlog.so* /usr/local/lib/

RUN ldconfig

# Create non-root user
RUN groupadd -r newsapp && useradd -r -g newsapp newsapp
RUN mkdir -p /app/logs && chown -R newsapp:newsapp /app

WORKDIR /app
USER newsapp

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=10s --start-period=30s --retries=3 \
    CMD curl -f http://localhost:8080/api/v1/health || exit 1

ENTRYPOINT ["/usr/local/bin/news_server"]
