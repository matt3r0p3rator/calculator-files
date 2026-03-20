#!/bin/bash
set -e

export PATH=/root/Ndless/ndless-sdk/toolchain/install/bin:/root/Ndless/ndless-sdk/bin:/root/Ndless/ndless-sdk/tools:$PATH

APPDIR=/mnt/c/Users/gigab/calculator-files/app_manager

rm -f "$APPDIR"/src/*.o "$APPDIR"/*.elf "$APPDIR"/*.tns "$APPDIR"/*.zehn
make -C "$APPDIR" && echo "=== BUILD SUCCESS: app_manager.tns ===" || { echo "=== BUILD FAILED ==="; exit 1; }
