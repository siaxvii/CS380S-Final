#!/bin/bash -e

SCRIPT_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))
REPO_DIR=$(dirname $SCRIPT_DIR)

BUILD_DIR=$REPO_DIR/build
INSTALL_DIR=$REPO_DIR/install

rm -rf $BUILD_DIR $INSTALL_DIR
