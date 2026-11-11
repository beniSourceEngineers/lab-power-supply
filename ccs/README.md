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
| `cpu1/src/hal/` | CPU1-only HAL: ADC, ePWM time base, power stage PWM, trip zone, CMPSS |
| `cpu1/src/app/` | CPU1 application: `IStateMachine`, open-loop task, scaling |
| `cpu1/cmd/`, `cpu2/cmd/` | Linker command files (from C2000Ware, owned by this repo) |
| `cpu*/targetConfigs/` | Debug probe configuration (XDS100v2) |
| `cpu*/.theia/launch.json` | Debug configurations: load both cores on the LaunchPad (RAM, FLASH, FLASH stand-alone); the same file in both projects |
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

CCS adds every imported project as its own root folder, so the workspace is a
multi-root workspace. CCS 20.0.1 then reads the debug configurations from
`<project>/.theia/launch.json` of the selected project. If a project has no
such file, CCS writes a `"launch"` block into the `.theia-workspace` file
instead and later rejects that file with *'launch.json' is not a valid JSON
file. Missing debug configuration properties.* Every project in this repo
therefore has its own `.theia/launch.json`.

If you see this error, close CCS, remove the `"launch"` block (and a top-level
`"configurations"` entry, if present) from the workspace file in
`~/.config/Texas Instruments/CCS/ccs2001/0/theia/workspaces/`, and start CCS
again.

## Run on the LaunchPad

The debug configurations are committed in `cpu1/.theia/launch.json` and
`cpu2/.theia/launch.json` (same content, so it does not matter which project
is selected). Each one
connects through `cpu1/targetConfigs/TMS320F28379D.ccxml` (XDS100v2) and loads
`labpsu_cpu1.out` on CPU1 and `labpsu_cpu2.out` on CPU2. Build both projects in
the matching configurations first, then pick the configuration in the *Run and
Debug* view and start it:

| Debug configuration | CPU1 build | CPU2 build | Use |
|---|---|---|---|
| `LaunchPad RAM (CPU1 + CPU2)` | `CPU1_RAM` | `CPU2_RAM` | Development |
| `LaunchPad FLASH (CPU1 + CPU2)` | `CPU1_FLASH` | `CPU2_FLASH` | Flash, with debugger |
| `LaunchPad FLASH stand-alone (CPU1 + CPU2)` | `CPU1_FLASH_STANDALONE` | `CPU2_FLASH` | Flash, runs after power-up without a debugger |

Run both cores. CPU1 configures the GPIOs and hands LED2 to CPU2; both cores
meet at `ipc::syncCores` (flag 31). In the stand-alone build, CPU1 boots CPU2
from flash itself (`Device_bootCPU2`).

The launch files are shared: change them only for settings everyone needs,
and keep both copies equal.

Sign of life: LED1 (CPU1) toggles every 500 ms, LED2 (CPU2) every 250 ms, both
from a 1 kHz CPU Timer 0 interrupt.

## Power stage (CPU1, Phase 6)

Hardware path, taken over from Project A (`LabPSU_CPU01.slx`, `LabPsu_Pwm`):

| Function | Peripheral | Set-up |
|---|---|---|
| Power stage PWM | ePWM4A/B on GPIO6/GPIO7 (LaunchPad pins 80/79) | 50 kHz up-down, TBPRD 1000, complementary, dead band 20 half cycles = 100 ns |
| Current measurement | ADC-A SOC0, ADCINA0 (LaunchPad J3 pin 30) | triggered by ePWM4 SOCA at counter = period, every 5th period (10 kHz) |
| Voltage measurement | ADC-B SOC0, ADCINB0 (not on a LaunchPad header) | triggered by ePWM5 SOCA, ePWM5 synced to ePWM4 with phase 50 counts |
| Control ISR, 10 kHz | ADCB1 (end of the voltage conversion) | reads both results |
| Off / Fault | ePWM4 one-shot trip, outputs forced low | tripped at start-up |

Differences to Project A:

- No HRPWM (and no SFO calibration) yet; the duty resolution is 1/1000.
- One control ISR on ADCB1 instead of Project A's interrupt block. Project A
  maps `CurrentControlTask` to PIE 1.3 (ADCC1), which no ADC raises, and
  `VoltageControlTask` to PIE 1.1 (ADCA1); to be clarified in Phase 7.
- `hal::Cmpss` and `TripZone::enableDigitalCompareTrip()` are ready for a
  hardware over-limit trip, but not used: ADCINA0/ADCINB0 are no CMPSS inputs.
- The measurement scaling (`raw / 4096 * 5`, as in Project A) is a placeholder.

### Open loop on the LaunchPad

Load `CPU1_RAM` and `CPU2_RAM` (*LaunchPad RAM (CPU1 + CPU2)*), run both, then
in the *Expressions* view:

| Expression | Use |
|---|---|
| `openLoopCommand.outputEnable` | 1 = clear the trip and drive the PWM, 0 = trip |
| `openLoopCommand.duty` | duty cycle of ePWM4A, 0 .. 1 |
| `openLoopStatus` | `outputEnabled`, `measurements`, `controlIsrCount` (10 kHz), `adcOverflowCount` |

`adcOverflowCount` counts up each time the debugger halts CPU1, because the
ePWMs keep running (free run) and the ADC keeps converting. It must not grow
while the CPU runs.

## Headless build

```sh
cd ~/ti/ccs2001/ccs/eclipse      # the CLI must be started from its own folder
./ccs-server-cli.sh -noSplash -workspace <ws> -application projectImport \
    -ccs.location <repo>/ccs/cpu1
./ccs-server-cli.sh -noSplash -workspace <ws> -application projectBuild \
    -ccs.projects labpsu_cpu1 -ccs.configuration CPU1_RAM
```

## IPC

`common/ipc/` holds the channels between the cores (AGENTS.md §6). To add one:

1. Put the message type in `common/shared_types/`, with a
   `STATIC_ASSERT_SIZE_WORDS` check, and allocate its flag in `IpcFlags.hpp`.
2. Add its storage to the channel struct of that direction. There is one struct
   per direction, shared by both cores, and each is the only object in its section
   (`common/ipc/MsgRamSections.hpp`). A queue has storage in both structs.
3. In each `main.cpp`, create the `ipc::Mailbox` or `ipc::Queue` on that storage.
   Initialise that core's side before `ipc::syncCores` and use it only afterwards.

The message RAM map is in `common/ipc/msgram_memory.inc` and
`msgram_sections.inc`, which the linker command files of both cores include.
`test/target/README.md` describes the IPC test on the LaunchPad.

## Host tests

The classes in `common/` are unit tested on the PC (`test/`, see AGENTS.md §5):

```sh
cmake -S test -B build/test && cmake --build build/test && ctest --test-dir build/test
```
