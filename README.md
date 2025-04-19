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

| Option             | Description                                                  |
|--------------------|--------------------------------------------------------------|
| `-d`, `--device`   | Target CD-ROM block device (default: `/dev/sr0`)             |
| `-s`, `--speed`    | Speed multiplier (e.g., `1`, `2`, `4`)                       |
| `-g`, `--sg`       | Optional SCSI generic device for fallback (`/dev/sgX`)       |
| `-r`, `--retry`    | Retry for N seconds if device not immediately available      |
| `-q`, `--quiet`    | Suppress all non-error output                                |
| `-v`, `--verbose`  | Enable debug output                                          |
| `-h`, `--help`     | Display usage                                                |


## Building

### From Source

```bash
make
sudo make install
```

### Packaging `.deb` (Debian/Ubuntu/Raspbian)

```bash
dpkg-buildpackage -b -us -uc
```

This produces a `.deb` in the parent directory.


## Example Integration

Use as a pre-step to CD audio playback in scripts or services:

```bash
cdspeedctl --device /dev/sr0 --sg /dev/sg1 --speed 1 --retry 5 --quiet
cdparanoia -B
```

## Return Codes

| Code | Meaning                              |
|------|--------------------------------------|
| 0    | Speed set successfully               |
| 1    | Device or media not ready            |
| 2    | Speed setting unsupported            |
| 3    | General or unknown error             |
| 64+  | Invalid argument or usage error      |


## License

MIT or BSD-style license. See `LICENSE` file for details.


## Future Work

- Auto-detection of `/dev/sgX` mapping
- Optional `udev` hook generator
- Query current drive speed (if supported)

## Maintainer

Just a Nerd  
GitHub: [github.com/foonerd](https://github.com/foonerd)
