FROM alpine:3.20 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    git \
    boost-dev \
    boost-static \
    openssl-dev \
    openssl-libs-static \
    curl-dev \
    curl-static \
    zlib-dev \
    zlib-static \
    nghttp2-static \
    brotli-static \
    zstd-static \
    libssh2-static \
    libidn2-static \
    libpsl-static \
    ninja

WORKDIR /app
COPY include ./include
COPY libs ./libs
COPY src ./src
COPY CMakeLists.txt .

RUN mkdir -p build
RUN cmake -S . -B build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXE_LINKER_FLAGS="-static" \
        -DCMAKE_CXX_FLAGS="-static-libstdc++ -static-libgcc" \
        -DBUILD_SHARED_LIBS=OFF
RUN cmake --build build -j$(proc)
RUN strip build/tgBot
