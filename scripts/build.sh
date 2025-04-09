#!/bin/bash -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
REPO_DIR=$(dirname $SCRIPT_DIR)

BUILD_DIR=$REPO_DIR/build
INSTALL_DIR=$REPO_DIR/install

cmake \
    -G Ninja \
    -DCMAKE_COLOR_DIAGNOSTICS=ON \
    -S $REPO_DIR \
    -B $BUILD_DIR \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
    -DCMAKE_BUILD_TYPE=RelWithDebugInfo \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build $BUILD_DIR
cmake --install $BUILD_DIR
