#!/bin/bash
export PATH=/root/Ndless/ndless-sdk/toolchain/install/bin:/root/Ndless/ndless-sdk/bin:/root/Ndless/ndless-sdk/tools:$PATH
SRCDIR=/mnt/c/Users/gigab/calculator-files/angband/src

make -C "$SRCDIR" -f Makefile.nspire && echo "=== BUILD SUCCESS: angband.tns ===" || echo "=== BUILD FAILED ==="
