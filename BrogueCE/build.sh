#!/bin/bash
# build.sh — Build BrogueCE as a TI-Nspire CX II .tns executable
#
# Usage:  ./build.sh
# Output: brogue.tns  (in the BrogueCE root directory)
#
# Prerequisites:
#   - Ndless SDK installed (default path set in NDLESS below)
#   - GNU make and the nspire-gcc cross-compiler

set -e

NDLESS=/home/matt3r/calculator-files/Ndless/ndless-sdk
export PATH=$NDLESS/toolchain/install/bin:$NDLESS/bin:$NDLESS/tools:$PATH

BROGUE_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== BrogueCE Nspire build ==="
echo "  BrogueCE root : $BROGUE_DIR"
echo "  Ndless SDK    : $NDLESS"
echo ""

make -C "$BROGUE_DIR" -f Makefile.nspire clean
make -C "$BROGUE_DIR" -f Makefile.nspire

if [ -f "$BROGUE_DIR/brogue.tns" ]; then
    echo ""
    echo "=== BUILD SUCCESS: brogue.tns ==="
    echo "Copy brogue.tns to /documents/ on the calculator."
    echo "On first run, /documents/brogue/ is created automatically."
else
    echo ""
    echo "=== BUILD FAILED ==="
    exit 1
fi
