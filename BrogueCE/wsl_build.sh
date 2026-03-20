#!/bin/bash
set -e

NDLESS=/root/Ndless/ndless-sdk
export PATH=$NDLESS/toolchain/install/bin:$NDLESS/bin:$NDLESS/tools:$PATH

BROGUE_DIR=/mnt/c/Users/gigab/calculator-files/BrogueCE

echo "=== BrogueCE Nspire build ==="
echo "  BrogueCE root : $BROGUE_DIR"
echo "  Ndless SDK    : $NDLESS"
echo ""

make -C "$BROGUE_DIR" -f Makefile.nspire clean
make -C "$BROGUE_DIR" -f Makefile.nspire && echo "=== BUILD SUCCESS: brogue.tns ===" || { echo "=== BUILD FAILED ==="; exit 1; }

if [ -f "$BROGUE_DIR/brogue.tns" ]; then
    echo ""
    echo "Copy brogue.tns to /documents/ on the calculator."
fi
