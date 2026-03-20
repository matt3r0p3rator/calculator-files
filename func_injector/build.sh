#!/bin/bash
# Packages init_funcs.lua as a .tns file the TI-Nspire will accept.
# The Nspire file browser runs .lua.tns files through the built-in Lua VM.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC="$SCRIPT_DIR/init_funcs.lua"
DST="$SCRIPT_DIR/init_funcs.lua.tns"

cp "$SRC" "$DST"
echo "=== Built: $DST ==="
echo ""
echo "Transfer init_funcs.lua.tns to your calculator."
echo "Open it from the file browser — it will define all functions"
echo "in the active document/scratchpad via math.eval()."
