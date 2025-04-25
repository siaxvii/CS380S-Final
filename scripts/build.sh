#!/bin/bash -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
REPO_DIR=$(dirname $SCRIPT_DIR)

RLBOX_WASM2C_PATH=$REPO_DIR/submodules/rlbox-wasm2c-sandbox
WASI_SDK_PATH=$RLBOX_WASM2C_PATH/build/_deps/wasiclang-src
INSTALL_DIR=$REPO_DIR/install

GREEN='\033[0;32m'
NC='\033[0m'

# Build RLBox
if [[ ! -d $REPO_DIR/submodules/rlbox-wasm2c-sandbox/build ]]; then
    cd $REPO_DIR/submodules/rlbox-wasm2c-sandbox

    echo -e "${GREEN}Building RLBox.${NC}"
    cmake -G Ninja -S . -B ./build
    cmake --build ./build --target all

    echo -e "${GREEN}Building WASM2C Sandbox Wrapper.${NC}"
    $WASI_SDK_PATH/bin/clang --sysroot $WASI_SDK_PATH/share/wasi-sysroot $RLBOX_WASM2C_PATH/c_src/wasm2c_sandbox_wrapper.c -c -o $RLBOX_WASM2C_PATH/c_src/wasm2c_sandbox_wrapper.o
fi

# Build FFmpeg
cd $REPO_DIR/submodules/ffmpeg

echo -e "${GREEN}Building FFmpeg.${NC}"

./configure \
    --ar=$WASI_SDK_PATH/bin/ar \
    --cc=$WASI_SDK_PATH/bin/clang \
    --cxx=$WASI_SDK_PATH/bin/clang++ \
    --ld=$WASI_SDK_PATH/bin/wasm-ld \
    --extra-cflags="--sysroot=$WASI_SDK_PATH/share/wasi-sysroot" \
    --extra-cxxflags="--sysroot=$WASI_SDK_PATH/share/wasi-sysroot" \
    --extra-ldflags="--export-all --stack-first -z stack-size=262144 --no-entry --growable-table --import-memory --import-table" \
    --extra-libs="$WASI_SDK_PATH/share/wasi-sysroot/lib/wasm32-wasi/libc.a $RLBOX_WASM2C_PATH/c_src/wasm2c_sandbox_wrapper.o" \
    --target-os=none \
    --arch=wasm32 \
    --disable-x86asm \
    --disable-inline-asm \
    --disable-programs \
    --disable-doc \
    --disable-debug \
    --enable-cross-compile \
    --disable-network \
    --disable-everything \
    --enable-decoder=pcm_s16le \
    --enable-demuxer=wav \
    --prefix=$INSTALL_DIR

make
make install
