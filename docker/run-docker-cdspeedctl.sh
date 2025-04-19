#!/bin/bash
set -e

VERBOSE=0
if [[ "$4" == "--verbose" ]]; then
  VERBOSE=1
fi

COMPONENT=cdspeedctl
ARCH=$1
MODE=$2  # optional: 'volumio'

if [[ -z "$ARCH" ]]; then
  echo "Usage: $0 <arch> [volumio] [--verbose]"
  echo "Example: $0 armv6"
  exit 1
fi

DOCKERFILE="docker/Dockerfile.cdspeedctl.$ARCH"
IMAGE_TAG="cdspeedctl-build-$ARCH"

declare -A ARCH_FLAGS
ARCH_FLAGS=(
  ["armv6"]="linux/arm/v7 --build=arm-linux-gnueabihf --host=arm-linux-gnueabihf"
  ["armhf"]="linux/arm/v7 --build=arm-linux-gnueabihf --host=arm-linux-gnueabihf"
  ["arm64"]="linux/arm64 --build=aarch64-linux-gnu --host=aarch64-linux-gnu"
  ["amd64"]="linux/amd64 --build=x86_64-linux-gnu --host=x86_64-linux-gnu"
)

if [[ -z "${ARCH_FLAGS[$ARCH]}" ]]; then
  echo "Error: Unknown architecture: $ARCH"
  exit 1
fi

PLATFORM="${ARCH_FLAGS[$ARCH]%% *}"
BUILD_HOST_FLAGS="${ARCH_FLAGS[$ARCH]#* }"

if [[ ! -f "$DOCKERFILE" ]]; then
  echo "Missing Dockerfile: $DOCKERFILE"
  exit 1
fi

echo "[+] Building Docker image for $ARCH ($PLATFORM)..."
if [[ "$VERBOSE" -eq 1 ]]; then
  DOCKER_BUILDKIT=1 docker build --platform=$PLATFORM --progress=plain -t $IMAGE_TAG -f $DOCKERFILE .
else
  docker build --platform=$PLATFORM --progress=auto -t $IMAGE_TAG -f $DOCKERFILE .
fi

echo "[+] Running build for $COMPONENT in Docker ($ARCH)..."
if [[ "$ARCH" == "armv6" ]]; then
  docker run --rm --platform=$PLATFORM -v "$PWD":/build -w /build $IMAGE_TAG bash -c "    export CFLAGS='-O2 -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm' &&     export CXXFLAGS=\"\$CFLAGS\" &&     dpkg-buildpackage -us -uc -b"
else
  docker run --rm --platform=$PLATFORM -v "$PWD":/build -w /build $IMAGE_TAG bash -c "    dpkg-buildpackage -us -uc -b"
fi

mkdir -p out/$ARCH
find . -maxdepth 1 -type f -name '*.deb' -exec mv {} out/$ARCH/ \;

if [[ "$MODE" == "volumio" ]]; then
  echo "[+] Volumio mode: Renaming .deb packages for custom suffixes..."
  for f in out/$ARCH/*.deb; do
    if [[ "$f" == *_all.deb ]]; then continue; fi
    base_name=$(basename "$f")
    newf="$f"
    case "$ARCH" in
      armv6)  newf="${f/_armhf.deb/_arm.deb}" ;;
      armhf)  newf="${f/_armhf.deb/_armv7.deb}" ;;
      arm64)  newf="${f/_arm64.deb/_armv8.deb}" ;;
      amd64)  newf="${f/_amd64.deb/_x64.deb}" ;;
    esac
    if [[ "$f" != "$newf" ]]; then mv "$f" "$newf"; fi
  done
fi

echo "[OK] Build complete. Packages are in out/$ARCH/"
