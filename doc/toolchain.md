# Toolchain

Pinned tool versions for both projects. Change them only in a dedicated PR, and
rebuild and re-verify both projects afterwards.

## MATLAB / Simulink (Project A and control code generation)

Host: Windows 11 Pro (build 26200)

| Product | Version |
|---|---|
| MATLAB | R2025b (25.2) Update 5 |
| Simulink | R2025b |
| Embedded Coder | R2025b |
| Simulink Coder | R2025b |
| MATLAB Coder | R2025b |
| C2000 Microcontroller Blockset | R2025b |
| Stateflow | R2025b |
| Simulink Test | R2025b |
| Control System Toolbox | R2025b |
| DSP System Toolbox | R2025b |
| Signal Processing Toolbox | R2025b |
| Simscape / Simscape Electrical | R2025b (plant models only, not used for code generation) |

## TI tools

MathWorks verifies these versions for the C2000 Microcontroller Blockset R2025b
([source](https://www.mathworks.com/help/ti-c2000/gs/CCS-MATLAB-versions.html)):

| Tool | Version | Used by |
|---|---|---|
| Code Composer Studio | 20.0.1 (CCS 20 only; older CCS versions are not supported) | Project A and B |
| C2000Ware | 5.02.00.00 | Project A (blockset install), Project B (copy in `ccs/ext/c2000ware/`) |
| TI C28x Code Generation Tools | 22.6.1.LTS | Project A and B |
| SysConfig | 1.22.0 (bundled with CCS 20.0.1) | Project B |

Status: **confirmed** (2026-10-22). Compiler changed from 22.6.0.LTS to
22.6.1.LTS on 2026-10-29: CCS 20.0.1 installs 22.6.1.LTS, a patch release of
the version MathWorks lists. Both projects use the compiler from the CCS
installation.

Reason: if both projects use the same C2000Ware and compiler, any difference in
the back-to-back comparison (Phase 8) comes from the workflow, not from the
tools. A newer C2000Ware (latest: 26.02.00.00 LTS with compiler 25.11.1.LTS) is
possible for Project B, but MathWorks has not verified it for R2025b.

### C2000Ware installation (Linux)

| | |
|---|---|
| Installer | `C2000Ware_5_02_00_00_setup.run` |
| SHA-256 | `f1fcb206ce9aa653267522b489707d9a7bf86c439693f6f2fdf288ac6cb5d3aa` |
| Install path | `~/ti/c2000/C2000Ware_5_02_00_00` |

The installer is a 32-bit (i386) InstallBuilder binary. On current Linux
distributions (e.g. Ubuntu with glibc 2.43, kernel 7.0) it fails with *"Unable
to initialize installer"* or crashes with a segmentation fault, even with
`libc6:i386` installed. Run it in an Ubuntu 20.04 container instead (rootless
podman, no sudo needed):

```sh
mkdir -p ~/ti/c2000
podman run --rm \
  -v ~/Downloads/C2000Ware_5_02_00_00_setup.run:/inst/setup.run:ro,Z \
  -v ~/ti/c2000:/out:Z \
  docker.io/library/ubuntu:20.04 bash -c '
    dpkg --add-architecture i386 && apt-get update -qq &&
    DEBIAN_FRONTEND=noninteractive apt-get install -y -qq libc6:i386 &&
    /inst/setup.run --mode unattended --prefix /out'
```

The installer creates `C2000Ware_5_02_00_00/` below the prefix. The subset that
Project B uses is copied into `ccs/ext/c2000ware/` (see its `README.md`).

### CCS installation (Linux)

| | |
|---|---|
| Installer | `CCS_20.0.1.00004_linux.zip` (offline, 1 008 341 356 bytes) |
| SHA-256 | `16a58676bd48401f2963e8fe380857879ed4ff56dd5f8a3a00c7f63dad0ad428` |
| Install path | `~/ti/ccs2001` |
| Components | `PF_C28` only |

Install the libraries that CCS and the debug probes need, then run the
installer unattended:

```sh
sudo apt install -y libc6-i386 libusb-0.1-4
unzip CCS_20.0.1.00004_linux.zip -d ccs20 && cd ccs20/CCS_20.0.1.00004_linux
./ccs_setup_20.0.1.00004.run --mode unattended \
  --prefix ~/ti/ccs2001 --enable-components PF_C28
sudo ~/ti/ccs2001/ccs/install_scripts/install_drivers.sh   # udev rules for XDS110 / J-Link
```

Notes:

- `libgconf-2-4` (listed for older CCS versions) does not exist on current
  Ubuntu and is not needed by CCS 20.
- The installer brings the C28x compiler
  (`ccs/tools/compiler/ti-cgt-c2000_22.6.1.LTS`) and SysConfig
  (`ccs/utils/sysconfig_1.22.0`).
- Call the headless CLI `ccs/eclipse/ccs-server-cli.sh` from its own directory.
  Started from another working directory, it fails with `node: not found`.
- On Ubuntu 24.04 and later, CCS (`ccs/theia/ccstudio`, an Electron app) crashes
  at start with *"The SUID sandbox helper binary was found, but is not
  configured correctly"*. AppArmor blocks unprivileged user namespaces
  (`kernel.apparmor_restrict_unprivileged_userns = 1`), which the Chromium
  sandbox needs. Start CCS with `--no-sandbox`, and add the flag to the menu
  entry in `~/.local/share/applications/CCS 20.0.1.desktop` (absolute path;
  `.desktop` files do not expand `~` or `$HOME`):

  ```
  Exec=/home/<user>/ti/ccs2001/ccs/theia/ccstudio --no-sandbox
  ```

  This turns the Chromium sandbox off. A reinstall or update of CCS may
  overwrite the menu entry. An alternative that keeps the sandbox is an
  AppArmor profile that allows `userns` for `ccstudio` (as Ubuntu ships for
  VS Code and Chrome).

## Third party

| Component | Version | Notes |
|---|---|---|
| CANopen stack | tbd | Licensed, later phase (see AGENTS.md §3.5) |
