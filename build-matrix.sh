#!/bin/bash
set -e

# Supported architectures
ARCHS=("armv6" "armhf" "arm64" "amd64")
VOL_MODE=""
VERBOSE=""

# Parse optional args
for arg in "$@"; do
  case "$arg" in
    --volumio) VOL_MODE="volumio" ;;
    --verbose) VERBOSE="--verbose" ;;
  esac
done

# Run build per architecture
for ARCH in "${ARCHS[@]}"; do
  echo "====================================="
  echo ">> Building for architecture: $ARCH"
  echo "====================================="
  ./docker/run-docker-cdspeedctl.sh "$ARCH" "$VOL_MODE" "$VERBOSE"
done

echo ""
echo "[✓] All builds completed. Check the 'out/' directory for results."
