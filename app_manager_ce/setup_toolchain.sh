#!/bin/bash
set -e
CEDEV_DIR=/root/CEdev

echo "==> Downloading CE toolchain..."
curl -sL https://github.com/CE-Programming/toolchain/releases/latest/download/CEdev-Linux.tar.gz -o /tmp/CEdev-Linux.tar.gz

echo "==> Extracting..."
mkdir -p /tmp/cedev_extract
tar -xzf /tmp/CEdev-Linux.tar.gz -C /tmp/cedev_extract
# The tarball may or may not have a top-level CEdev/ folder
if [ -d /tmp/cedev_extract/CEdev ]; then
    cp -r /tmp/cedev_extract/CEdev "$CEDEV_DIR"
else
    cp -r /tmp/cedev_extract "$CEDEV_DIR"
fi

echo "==> Setting env vars..."
# Remove any stale CEDEV lines first
grep -v 'CEDEV\|CEdev/bin' ~/.bashrc > /tmp/bashrc_clean || true
cp /tmp/bashrc_clean ~/.bashrc
echo "export CEDEV=$CEDEV_DIR" >> ~/.bashrc
echo "export PATH=\$PATH:$CEDEV_DIR/bin" >> ~/.bashrc

echo "==> Verifying..."
export CEDEV=$CEDEV_DIR
export PATH=$PATH:$CEDEV_DIR/bin
ls "$CEDEV_DIR/bin/" | head -5
which ez80-clang || echo "WARNING: ez80-clang not found in bin"

echo "==> Building app_manager_ce..."
cd /mnt/c/Users/gigab/calculator-files/app_manager_ce
make 2>&1

echo "==> Done."
