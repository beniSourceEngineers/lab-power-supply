# Project B: CCS projects (C++03)

Two Code Composer Studio 20 projects for the F28379D, one per core
(AGENTS.md §3.4):

| Project | Folder | Role | Build configurations |
|---|---|---|---|
| `labpsu_cpu1` | `cpu1/` | System init, CPU2 boot, peripheral hand-over, control | `CPU1_RAM`, `CPU1_FLASH`, `CPU1_FLASH_STANDALONE` |
| `labpsu_cpu2` | `cpu2/` | Communication | `CPU2_RAM`, `CPU2_FLASH` |

Both target the LAUNCHXL-F28379D (`_LAUNCHXL_F28379D`: 10 MHz crystal, LED1 =
GPIO31, LED2 = GPIO34, on-board XDS100v2 probe).

## Layout

| Path | Contents |
|---|---|
| `cpu1/src/`, `cpu2/src/` | `main.cpp` (composition root) and core-specific code |
| `cpu1/cmd/`, `cpu2/cmd/` | Linker command files (from C2000Ware, owned by this repo) |
| `cpu*/targetConfigs/` | Debug probe configuration (XDS100v2) |
| `common/` | Code for both cores, linked into both projects as folder `common` |
| `ext/c2000ware/` | C2000Ware copy; driverlib and `device.c` are linked in as folder `c2000ware` and built from source |

The project files (`.project`, `.cproject`, `.ccsproject`) are committed and
the projects live in place. New files you create in CCS go into the repo.
Build output goes to one folder per configuration (`cpu1/CPU1_RAM/`, ...) and
is ignored by git.

## Open in CCS

1. *File → Import Projects…*, select the folder `ccs/` of this repo.
2. Select `labpsu_cpu1` and `labpsu_cpu2`. Do **not** copy the projects into the
   workspace.
3. Build each project (*Project → Build Project*). Pick the build configuration
   in the project's context menu.

## Run on the LaunchPad

- **RAM** (development): build `CPU1_RAM` and `CPU2_RAM`, start a debug session
  with `cpu1/targetConfigs/TMS320F28379D.ccxml`, load `labpsu_cpu1.out` on
  CPU1 and `labpsu_cpu2.out` on CPU2, run both. CPU1 configures the GPIOs and
  hands LED2 to CPU2; both cores meet at `IPC_sync` (flag 31).
- **Flash, with debugger**: `CPU1_FLASH` + `CPU2_FLASH`, load and run both
  cores as above.
- **Flash, stand-alone**: `CPU1_FLASH_STANDALONE` + `CPU2_FLASH`. CPU1 boots CPU2
  from flash itself (`Device_bootCPU2`), so the board runs after power-up
  without a debugger.

Sign of life: LED1 (CPU1) toggles every 500 ms, LED2 (CPU2) every 250 ms, both
from a 1 kHz CPU Timer 0 interrupt.

## Headless build

```sh
cd ~/ti/ccs2001/ccs/eclipse      # the CLI must be started from its own folder
./ccs-server-cli.sh -noSplash -workspace <ws> -application projectImport \
    -ccs.location <repo>/ccs/cpu1
./ccs-server-cli.sh -noSplash -workspace <ws> -application projectBuild \
    -ccs.projects labpsu_cpu1 -ccs.configuration CPU1_RAM
```

## Host tests

The classes in `common/` are unit tested on the PC (`test/`, see AGENTS.md §5):

```sh
cmake -S test -B build/test && cmake --build build/test && ctest --test-dir build/test
```
