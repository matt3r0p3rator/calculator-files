#!/bin/bash
export PATH=/root/Ndless/ndless-sdk/toolchain/install/bin:/root/Ndless/ndless-sdk/bin:/root/Ndless/ndless-sdk/tools:$PATH
APPDIR=/mnt/c/Users/gigab/calculator-files/app_manager

rm -f "$APPDIR"/src/*.o "$APPDIR"/app_manager.elf "$APPDIR"/app_manager.tns
make -C "$APPDIR" && echo "=== BUILD SUCCESS: app_manager.tns ===" || echo "=== BUILD FAILED ==="
