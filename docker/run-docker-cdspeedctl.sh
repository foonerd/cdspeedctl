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

# Clean previous build staging area
rm -rf build/$COMPONENT/source
mkdir -p build/$COMPONENT/source
mkdir -p out/$ARCH

# Copy project files to isolated build area
cp -r src build/$COMPONENT/source/
cp -r debian build/$COMPONENT/source/
cp -f Makefile build/$COMPONENT/source/

echo "[+] Building Docker image for $ARCH ($PLATFORM)..."
if [[ "$VERBOSE" -eq 1 ]]; then
  DOCKER_BUILDKIT=1 docker build --platform=$PLATFORM --progress=plain -t $IMAGE_TAG -f $DOCKERFILE .
else
  docker build --platform=$PLATFORM --progress=auto -t $IMAGE_TAG -f $DOCKERFILE .
fi

echo "[+] Running build for $COMPONENT in Docker ($ARCH)..."
if [[ "$ARCH" == "armv6" ]]; then
  docker run --rm --platform=$PLATFORM -v "$PWD":/build -w /build/build/$COMPONENT/source $IMAGE_TAG bash -c '\
    export CFLAGS="-O2 -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm" && \
    export CXXFLAGS="$CFLAGS" && \
    dpkg-buildpackage -us -uc -b'
else
  docker run --rm --platform=$PLATFORM -v "$PWD":/build -w /build/build/$COMPONENT/source $IMAGE_TAG bash -c "\
    dpkg-buildpackage -us -uc -b"
fi

# Move .deb files out of build tree into persistent output dir
find build/$COMPONENT -maxdepth 1 -type f -name '*.deb' -exec mv {} out/$ARCH/ \;

# Volumio-specific renaming rules
if [[ "$MODE" == "volumio" ]]; then
  echo "[+] Volumio mode: Renaming .deb packages for custom suffixes..."
  for f in out/$ARCH/*.deb; do
    [[ "$f" == *_all.deb ]] && continue
    base_name=$(basename "$f")
    new_name="$base_name"

    case "$ARCH" in
      armv6)
        new_name="${base_name/_armhf.deb/_arm.deb}"
        [[ "$base_name" != "$new_name" ]] && echo "[VERBOSE] Renaming to $new_name (ARMv6/7 target VFP2 - hard-float)"
        ;;
      armhf)
        new_name="${base_name/_armhf.deb/_armv7.deb}"
        [[ "$base_name" != "$new_name" ]] && echo "[VERBOSE] Renaming to $new_name (ARMv7 target)"
        ;;
      arm64)
        new_name="${base_name/_arm64.deb/_armv8.deb}"
        [[ "$base_name" != "$new_name" ]] && echo "[VERBOSE] Renaming to $new_name (ARMv8 target)"
        ;;
      amd64)
        new_name="${base_name/_amd64.deb/_x64.deb}"
        [[ "$base_name" != "$new_name" ]] && echo "[VERBOSE] Renaming to $new_name (x86_64 target)"
        ;;
    esac

    if [[ "$base_name" != "$new_name" ]]; then
      mv "out/$ARCH/$base_name" "out/$ARCH/$new_name"
    fi
  done
fi

echo "[OK] Build complete. Packages are in out/$ARCH/"
