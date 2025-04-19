#!/bin/bash
set -e

echo "[+] Cleaning up build artifacts..."

# Remove built binaries
make clean || true

# Clean Debian build files
rm -f ../cdspeedctl_*.deb ../cdspeedctl-dbgsym_*.deb ../*.buildinfo ../*.changes

# Remove Docker staging area
rm -rf build/cdspeedctl

# Remove per-arch output
rm -rf out/*

echo "[✓] Cleanup complete."
