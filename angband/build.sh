#!/bin/bash
NDLESS=/home/matt3r/calculator-files/Ndless/ndless-sdk
export PATH=$NDLESS/toolchain/install/bin:$NDLESS/bin:$NDLESS/tools:$PATH
SRCDIR=/home/matt3r/calculator-files/angband/src

make -C "$SRCDIR" -f Makefile.nspire clean
make -C "$SRCDIR" -f Makefile.nspire && echo "=== BUILD SUCCESS: angband.tns ===" || echo "=== BUILD FAILED ==="
if [ -f "$SRCDIR/angband.tns" ]; then
    cp "$SRCDIR/angband.tns" .
fi