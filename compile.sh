#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DEBUG_CONFIG_COMMAND="./waf configure --enable-examples --build-profile=debug --out=${SCRIPT_DIR}/build/debug"
OPT_CONFIG_COMMAND="./waf configure --enable-examples --build-profile=optimized --out=${SCRIPT_DIR}/build/optimized"
NUM_CORES=$(nproc --all)

if [ "$(uname)" == "Darwin" ]; then
    DEBUG_CONFIG_COMMAND="${DEBUG_CONFIG_COMMAND} --disable-werror"
    OPT_CONFIG_COMMAND="${OPT_CONFIG_COMMAND} --disable-werror"
fi

function buildDebug {
    cd ${SCRIPT_DIR} && ${DEBUG_CONFIG_COMMAND}
    cd ${SCRIPT_DIR} && ./waf build -j"${NUM_CORES}"
}

function buildOptimized {
    cd ${SCRIPT_DIR} && ${OPT_CONFIG_COMMAND}
    cd ${SCRIPT_DIR} && ./waf build -j"${NUM_CORES}"
}

if [ "$1" == "debug" ]
then
    buildDebug
elif [ "$1" == "optimized" ]
then
    buildOptimized
else
    buildDebug
    buildOptimized
fi
