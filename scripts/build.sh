#!/bin/bash -e

# Set variables

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
REPO_DIR=$(dirname $SCRIPT_DIR)

BUILD_DIR=$REPO_DIR/build
INSTALL_DIR=$REPO_DIR/install

GREEN='\033[0;32m'
NC='\033[0m'


RLBOX_BUILD_DIR=$BUILD_DIR-rlbox
RLBOX_ROOT=$REPO_DIR/submodules/rlbox-wasm2c-sandbox
RLBOX_INCLUDE=$RLBOX_BUILD_DIR/_deps/rlbox-src/code/include
WASI_SDK_ROOT=$RLBOX_BUILD_DIR/_deps/wasiclang-src
WASM2C_RUNTIME_PATH=$RLBOX_BUILD_DIR/_deps/wasm2c_compiler-src/wasm2c
WASI_RUNTIME_FILES="$WASM2C_RUNTIME_PATH/wasm-rt-impl.c $WASM2C_RUNTIME_PATH/wasm-rt-mem-impl.c"
WASM2C_RUNTIME_FILES="$RLBOX_ROOT/src/wasm2c_rt_minwasi.c $RLBOX_ROOT/src/wasm2c_rt_mem.c"
WASI_CLANG=$WASI_SDK_ROOT/bin/clang
WASI_SYSROOT=$WASI_SDK_ROOT/share/wasi-sysroot
WASM2C=$RLBOX_BUILD_DIR/_deps/wasm2c_compiler-src/bin/wasm2c
WASM_CFLAGS="-Wl,--export-all -Wl,--stack-first -Wl,-z,stack-size=262144 \
    -Wl,--no-entry -Wl,--growable-table -Wl,--import-memory -Wl,--import-table"

FFMPEG_INSTALL_DIR=$INSTALL_DIR-ffmpeg


# Build RLBox

if [[ ! -d $RLBOX_BUILD_DIR ]]; then
    cd $REPO_DIR/submodules/rlbox-wasm2c-sandbox

    echo -e "${GREEN}Building RLBox.${NC}"
    cmake -G Ninja -S . -B $RLBOX_BUILD_DIR
    cmake --build $RLBOX_BUILD_DIR

    echo -e "${GREEN}Building WASM2C Sandbox Wrapper.${NC}"
    $WASI_SDK_ROOT/bin/clang --sysroot $WASI_SYSROOT $RLBOX_ROOT/c_src/wasm2c_sandbox_wrapper.c -c -o $RLBOX_ROOT/c_src/wasm2c_sandbox_wrapper.o
fi


# Build FFmpeg

if [[ ! -d $FFMPEG_INSTALL_DIR ]]; then
    cd $REPO_DIR/submodules/ffmpeg

    echo -e "${GREEN}Building FFmpeg.${NC}"


    ./configure \
        --ar=$WASI_SDK_ROOT/bin/llvm-ar \
        --cc=$WASI_SDK_ROOT/bin/clang \
        --cxx=$WASI_SDK_ROOT/bin/clang++ \
        --ld=$WASI_SDK_ROOT/bin/wasm-ld \
        --nm=$WASI_SDK_ROOT/bin/llvm-nm \
        --ranlib=$WASI_SDK_ROOT/bin/ranlib \
        --strip=$WASI_SDK_ROOT/bin/llvm-strip \
        --extra-cflags="--sysroot=$WASI_SYSROOT -D_WASI_EMULATED_PROCESS_CLOCKS" \
        --extra-cxxflags="--sysroot=$WASI_SYSROOT -D_WASI_EMULATED_PROCESS_CLOCKS" \
        --extra-ldflags="--export-all --stack-first -z stack-size=262144 --no-entry --growable-table --import-memory --import-table" \
        --extra-libs="$WASI_SYSROOT/lib/wasm32-wasi/libc.a $WASI_SYSROOT/lib/wasm32-wasi/libwasi-emulated-process-clocks.a $RLBOX_ROOT/c_src/wasm2c_sandbox_wrapper.o" \
        --target-os=none \
        --arch=wasm32 \
        --disable-x86asm \
        --disable-inline-asm \
        --disable-programs \
        --disable-doc \
        --disable-debug \
        --enable-cross-compile \
        --disable-network \
        --disable-pthreads \
        --disable-everything \
        --enable-decoder=pcm_s16le \
        --enable-demuxer=flac \
        --enable-demuxer=mp3 \
        --enable-demuxer=mov \
        --enable-demuxer=ogg \
        --enable-demuxer=wav \
        --prefix=$FFMPEG_INSTALL_DIR

    make -j
    make install
fi


# Build Repo

echo -e "${GREEN}Building FFmpeg wrapper (WASM).${NC}"
$WASI_CLANG \
    --sysroot $WASI_SYSROOT \
    $WASM_CFLAGS \
    -I $FFMPEG_INSTALL_DIR/include \
    -L $FFMPEG_INSTALL_DIR/lib \
        -lwasi-emulated-process-clocks \
        -lavutil -lavcodec -lavformat -lavdevice \
        -lavfilter -lswresample -lswscale -lm \
    $REPO_DIR/src/ffmpeg_wrapper.c \
    -o $REPO_DIR/src/ffmpeg_wrapper.wasm

echo -e "${GREEN}Generating FFmpeg wrapper source (WASM -> C).${NC}"
rm -f $REPO_DIR/src/ffmpeg_wasm2c.c
$WASM2C $REPO_DIR/src/ffmpeg_wrapper.wasm -o $REPO_DIR/src/ffmpeg_wasm2c.c

echo -e "${GREEN}Compiling app with FFmpeg wrapper.${NC}"
rm -rf $REPO_DIR/build
mkdir $REPO_DIR/build
cd $REPO_DIR/build
gcc -c \
    $WASI_RUNTIME_FILES \
    $WASM2C_RUNTIME_FILES \
    -I $RLBOX_INCLUDE \
    -I $RLBOX_ROOT/include \
    -I $WASM2C_RUNTIME_PATH \
    $REPO_DIR/src/ffmpeg_wasm2c.c
g++ -std=c++17 \
    $REPO_DIR/src/main.cpp \
    -o $REPO_DIR/main \
    -I $RLBOX_INCLUDE \
    -I $RLBOX_ROOT/include \
    -I $WASM2C_RUNTIME_PATH *.o \
    -lpthread

rm *.o