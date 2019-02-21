#!/usr/bin/env bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function buildDebug {
    cd ${SCRIPT_DIR} && ./waf configure --enable-examples --build-profile=debug --out=${SCRIPT_DIR}/build/debug
    cd ${SCRIPT_DIR} && ./waf build
}

function buildOptimized {
    cd ${SCRIPT_DIR} && ./waf configure --enable-examples --build-profile=optimized --out=${SCRIPT_DIR}/build/optimized
    cd ${SCRIPT_DIR} && ./waf build
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
