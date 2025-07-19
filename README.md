# cdspeedctl

`cdspeedctl` is a lightweight, cross-architecture command-line tool for **setting CD-ROM drive speed** on Linux systems. It is designed to **mimic Windows behavior** by enforcing whisper-quiet mechanical playback of **Audio CDs**, particularly on **USB-connected optical drives** like the Pioneer UBEX series, used with **low-power single-board computers** (e.g., Raspberry Pi Zero).

## Features

- Sets drive read speed using:
  - `CDROM_SELECT_SPEED` via `ioctl()` (preferred)
  - SCSI MMC `SET CD SPEED (0xBB)` via `SG_IO` as fallback
- Supports `/dev/srX` and `/dev/sgX`
- Safe to call before every playback session
- Minimal resource footprint (no background daemons or polling)
- Designed for **bit-perfect playback with reduced drive noise**

## Usage

```bash
cdspeedctl --device /dev/sr0 --speed 1
```

### Options

| Option            | Description                                             |
| ----------------- | ------------------------------------------------------- |
| `-d`, `--device`  | Target CD-ROM block device (default: `/dev/sr0`)        |
| `-s`, `--speed`   | Speed multiplier (e.g., `1`, `2`, `4`)                  |
| `-g`, `--sg`      | Optional SCSI generic device for fallback (`/dev/sgX`)  |
| `-r`, `--retry`   | Retry for N seconds if device not immediately available |
| `-q`, `--quiet`   | Suppress all non-error output                           |
| `-v`, `--verbose` | Enable debug output                                     |
| `-c`, `--current` | Get the current speed of the CD-ROM drive               |
| `-h`, `--help`    | Display usage                                           |

## Building

### From Source (Native or Cross Environment)

Ensure build dependencies (e.g. `gcc`, `make`) are installed.

```bash
make
sudo make install
```

#### Overriding Compiler Flags

To build for a specific architecture (e.g. ARMv6 hard-float):

```bash
export CFLAGS="-O2 -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm"
make
```

You will see a summary like:

```text
[+] Building cdspeedctl with CC=gcc and CFLAGS=-O2 -march=armv6 -mfpu=vfp -mfloat-abi=hard -marm
```

### Verifying Output

You can confirm architecture and float ABI using:

```bash
file cdspeedctl
readelf -A cdspeedctl | grep Tag_ABI
```

Example output for ARMv6 hard-float:

```text
cdspeedctl: ELF 32-bit LSB executable, ARM, EABI5, hard-float
Tag_ABI_HardFP_use: Yes
```

### Packaging `.deb` (Debian/Ubuntu/Raspbian)

Builds a local Debian package:

```bash
dpkg-buildpackage -b -us -uc
```

This outputs `.deb` files in the parent directory (`../`).

### Docker-Based Cross-Architecture Packaging

To produce `.deb` packages for all target platforms (ARMv6, ARMv7, ARM64, AMD64):

```bash
./build-matrix.sh --volumio --verbose
```

This:

* Uses isolated Docker containers per target
* Copies source + packaging to `build/cdspeedctl/source/`
* Runs `dpkg-buildpackage` inside each container
* Outputs `.deb` packages to `out/<arch>/`

Renamed outputs for Volumio convention:

| Arch  | Original `.deb` Suffix | Renamed Suffix |
| ----- | ---------------------- | -------------- |
| armv6 | `_armhf.deb`           | `_arm.deb`     |
| armhf | `_armhf.deb`           | `_armv7.deb`   |
| arm64 | `_arm64.deb`           | `_armv8.deb`   |
| amd64 | `_amd64.deb`           | `_x64.deb`     |

## Cleaning

To remove all build artifacts, intermediate files, and `.deb` outputs:

```bash
./clean-all.sh
```

## Example Integration

Use as a pre-step to CD audio playback in scripts or services:

```bash
cdspeedctl --device /dev/sr0 --sg /dev/sg1 --speed 1 --retry 5 --quiet
cdparanoia -B
```

Alternatively, if you want to verify the current speed before playback:

```bash
cdspeedctl --device /dev/sr0 --current
cdspeedctl --device /dev/sr0 --sg /dev/sg1 --speed 1 --retry 5 --quiet
cdparanoia -B
```

This ensures that:

1. The current speed is queried first (if needed).
2. The speed is set via `cdspeedctl`.
3. **`cdparanoia -B`** is used for the actual playback or rip operation.


## Return Codes

| Code | Meaning                         |
| ---- | ------------------------------- |
| 0    | Speed set successfully          |
| 1    | Device or media not ready       |
| 2    | Speed setting unsupported       |
| 3    | General or unknown error        |
| 64+  | Invalid argument or usage error |

## License

MIT or BSD-style license. See `LICENSE` file for details.

## Future Work

* Auto-detection of `/dev/sgX` mapping
* Optional `udev` hook generator
* Query current drive speed (if supported)

## Maintainer

Just a Nerd
GitHub: [github.com/foonerd](https://github.com/foonerd)

---

### Key Updates:
1. **Added the `-c` flag** for querying the current speed of the CD-ROM drive.
2. **Retained original functionality** and options like `-s` for setting speed and retry logic.
3. **Cleaned up** the formatting and ensured all features are clearly documented. 


