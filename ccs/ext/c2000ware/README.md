# C2000Ware (copy for Project B)

Pinned copy of the parts of **TI C2000Ware 5.02.00.00** that the handwritten
F28379D code in `ccs/` needs (AGENTS.md §3.3). Only Project B uses it. Project A
uses the C2000Ware that the MathWorks C2000 Microcontroller Blockset installs.

Do not edit these files. To update, replace the whole copy with the steps below
in a dedicated PR and update `doc/toolchain.md`.

## Source

| | |
|---|---|
| Installer | `C2000Ware_5_02_00_00_setup.run` (Linux) |
| SHA-256 | `f1fcb206ce9aa653267522b489707d9a7bf86c439693f6f2fdf288ac6cb5d3aa` |
| License | `license.txt` (TI); source files are BSD-3-Clause, see `C2000Ware_5.02.00.00_manifest.html` |

## Contents

Paths are the same as in the C2000Ware installation, so paths relative to the
C2000Ware root work unchanged.

| Path | Contents |
|---|---|
| `device_support/f2837xd/common/` | Device support: `device.c/.h`, `F2837xD_*` sources, linker command files (`cmd/`), target configurations |
| `device_support/f2837xd/headers/` | Bit-field register headers, header linker command files |
| `driverlib/f2837xd/driverlib/` | driverlib sources and headers (without the prebuilt `ccs/` libraries; driverlib is built from source) |
| `license.txt`, `C2000Ware_5.02.00.00_manifest.html` | License and software manifest |

Not copied: examples, docs, other devices, libraries, utilities.

## Update procedure

```sh
# 1. Install C2000Ware (see doc/toolchain.md for the container workaround)
C2K=~/ti/c2000/C2000Ware_5_02_00_00
D=ccs/ext/c2000ware

# 2. Replace the copy
rm -rf "$D/device_support" "$D/driverlib"
mkdir -p "$D/device_support/f2837xd" "$D/driverlib/f2837xd"
cp -r "$C2K/device_support/f2837xd/common" "$C2K/device_support/f2837xd/headers" "$D/device_support/f2837xd/"
rsync -a --exclude=ccs "$C2K/driverlib/f2837xd/driverlib" "$D/driverlib/f2837xd/"
cp "$C2K/license.txt" "$C2K"/C2000Ware_*_manifest.html "$D/"
```
