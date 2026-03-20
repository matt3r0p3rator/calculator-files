#!/bin/bash
set -e

NDLESS=/root/Ndless/ndless-sdk
export PATH=$NDLESS/toolchain/install/bin:$NDLESS/bin:$NDLESS/tools:$PATH

SRCDIR=/mnt/c/Users/gigab/calculator-files/angband/src
OUTDIR=/mnt/c/Users/gigab/calculator-files/angband

make -C "$SRCDIR" -f Makefile.nspire clean
make -C "$SRCDIR" -f Makefile.nspire && echo "=== BUILD SUCCESS: angband.tns ===" || { echo "=== BUILD FAILED ==="; exit 1; }

if [ -f "$SRCDIR/angband.tns" ]; then
    cp "$SRCDIR/angband.tns" "$OUTDIR/"
fi
