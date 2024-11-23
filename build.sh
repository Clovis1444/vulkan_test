#!/bin/bash
# This script builds project
#
# Args:
# --target:<value>       [OPTIONAL]        build selected target
# --build:<value>        [OPTIONAL]        set build dir

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
cd "$SCRIPT_DIR" || exit 1

BUILD_DIR='./build'
C_C='-DCMAKE_C_COMPILER=clang'
CXX_C='-DCMAKE_CXX_COMPILER=clang++'
BUILD_GEN="-G Ninja"

TARGET=""
for arg in "$@"; do
    if [[ $arg == --target:* ]]; then
        TARGET="--target ${arg#--target:}"
    fi
    if [[ $arg == --build:* ]]; then
        BUILD_DIR="${arg#--build:}"
    fi
done

cmake -S . -B ${BUILD_DIR} ${C_C} ${CXX_C} ${BUILD_GEN} || exit 1
cmake --build ${BUILD_DIR} ${TARGET} || exit 1
