#!/bin/bash
SDK="$(cd "$(dirname "$0")/../../Ndless/ndless-sdk" && pwd)"
export PATH="$SDK/toolchain/install/bin:$SDK/bin:$PATH"
APPDIR="$(cd "$(dirname "$0")" && pwd)"

rm -f "$APPDIR"/src/*.o "$APPDIR"/app_manager.elf "$APPDIR"/app_manager.tns
make -C "$APPDIR" && echo "=== BUILD SUCCESS: app_manager.tns ===" || echo "=== BUILD FAILED ==="
