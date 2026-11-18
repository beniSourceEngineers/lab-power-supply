# SysConfig: LabPSU hardware setup

TI SysConfig description of the whole hardware setup of the lab power supply
on the LAUNCHXL-F28379D (F28379DZWTT, package 337ZWT), one script per core:

| File | Context | Contents |
|---|---|---|
| `labpsu_cpu1.syscfg` | CPU1 | ePWM1/4/5 (incl. HRPWM, dead band, ADC triggers), ADC A/B, CPU timer 0, LEDs, IPC1 interrupt, peripheral and GPIO hand-over to CPU2 |
| `labpsu_cpu2.syscfg` | CPU2 | SCIA (PC protocol), CPU timer 0, IPC flag 1 |
| `generate.sh` | both | Runs SysConfig for both cores in one call (multi-core mode) |

The settings mirror **Project A** as it runs on the target: the hardware
blocks in `simulink_full/LabPSU_CPU01.slx`, `simulink_full/LabPSU_CPU02.slx`,
`control/subsystems/LabPsu_Pwm.slx` and the configuration sets
`CpuOneTopLevel`/`CpuTwoTopLevel` in `control/data/configuration.sldd`. The
ePWM, ADC, GPIO and SCI values were checked against the registers read back
from the LaunchPad on 2026-11-18.

The files are a reference and a starting point for the HAL of Project B. They
are **not** part of the CCS build yet: `labpsu_cpu1`/`labpsu_cpu2` do not run
SysConfig, and the generated output is not committed.

## Generate

```sh
ccs/sysconfig/generate.sh            # output in ccs/sysconfig/generated/{cpu1,cpu2} (git-ignored)
```

Needs the SysConfig CLI from CCS 20.0.1 and the full C2000Ware 5.02.00.00
install (see `doc/toolchain.md`). The copy in `ccs/ext/c2000ware/` has no
SysConfig metadata. Override the paths with `SYSCONFIG_CLI` and
`C2000WARE_ROOT`.

Both scripts must run in **one** SysConfig call (`--script … --context CPU1
--script … --context CPU2`). Only CPU1 can write the GPIO mux, so CPU1's
`board.c` also muxes the pins of CPU2's SCIA (GPIO42/43). Generating a core on
its own loses that, and the IPC module reports an error.

Expected result: `0 error(s)`, about 33 warnings. The warnings are the HRPWM
checks below and hints from the LaunchPad board definition ("Connected to
hardware").

To edit in the GUI, open the `.syscfg` files in CCS 20 (SysConfig editor). The
`@cliArgs` line in each file sets board, device, package, context and product.

## Pins (LaunchPad headers)

| Signal | GPIO | Header | Core |
|---|---|---|---|
| EPWM1A / EPWM1B (HRPWM clock source) | GPIO0 / GPIO1 | J4-10 / J4-9 (40 / 39) | CPU1 |
| EPWM4A / EPWM4B (power stage) | GPIO6 / GPIO7 | J8-10 / J8-9 (80 / 79) | CPU1 |
| EPWM5A / EPWM5B (voltage ADC trigger) | GPIO8 / GPIO9 | J8-8 / J8-7 (78 / 77) | CPU1 |
| ADCINA0 (current measurement) | – | J3-10 (30) | CPU1 |
| ADCINB0 (voltage measurement) | – | not on a BoosterPack header (J3/J7 carry only B2–B5; see schematic sheet 9) | CPU1 |
| SCITXDA / SCIRXDA (PC link) | GPIO42 / GPIO43 | XDS100v2 virtual COM port (CON1) | CPU2 |
| LED D10 red | GPIO31 | on board | CPU1 |
| LED D9 blue | GPIO34 | on board | CPU2 |

## Timing

| Item | Setting |
|---|---|
| SYSCLK | 200 MHz (10 MHz crystal); not part of the scripts, set by `Device_init()` |
| EPWMCLK | SYSCLK/2 = 100 MHz, TBCLK = EPWMCLK |
| ePWM1 | up count, TBPRD 2000 → 50 kHz, CMPA 50, sync out disabled |
| ePWM4 | up-down, TBPRD 1000 → 50 kHz, CMPA from the current loop, complementary B with 20-count RED/FED at half-cycle clocking (100 ns), SOCA at CTR=PRD, sync out at CTR=0 |
| ePWM5 | up-down, TBPRD 1000 → 50 kHz, phase 50 after sync (count down), SOCA at CTR=PRD |
| ADC A/B | 12 bit single-ended, ADCCLK = SYSCLK/5 = 40 MHz, SOC0 = ADCIN0, sample window 15 |
| Interrupts CPU1 | TIMER0 (1 ms base rate), ADCA1 (EOC0 of ADC A), IPC1 (setpoints from CPU2) |
| Interrupts CPU2 | TIMER0 (1 ms base rate), SCIA RX FIFO level 1 (INT9.1) |
| SCIA | 115200 baud, 8N1, FIFO |

## Deviations from the target

SysConfig cannot express two details of the Simulink setup exactly:

- **ePWM1 compare load:** the target loads CMPA/CMPB at CTR=0 (`CMPCTL` = 0).
  With high-resolution period control SysConfig requires the load at CTR=PRD
  (error otherwise), so the script uses CTR=PRD.
- **GPIO9:** on the target GPIO9 stays a GPIO input. The SysConfig ePWM pin mux
  only knows the use case "ALL" (A and B), so GPIO9 is muxed to EPWM5B. With no
  B actions it drives low.

Also note for the script syntax: `hrpwm_enable = true` must come **before** the
`$assign` lines of an ePWM instance. Enabling HRPWM clears earlier peripheral
and pin assignments, and SysConfig then silently picks other ePWMs.

## Findings in the Simulink hardware setup

Found while building these scripts; the scripts reproduce the target as is.

1. **Current loop interrupt:** `LabPSU_CPU01.slx` calls *Current Control* from
   INT1.3 (ADCC1). ADC C is not configured, so that interrupt never fires. ADC A
   raises ADCINT1 → INT1.1, which calls *Voltage Control*; ADC B has no
   interrupt. Expected: current loop on INT1.1 (ADCA1), voltage loop on INT1.2
   (ADCB1) with ADCINT1 enabled on ADC B. (Seen in the generated code of
   2026-11-12 and in the current model.)
2. **HRPWM settings:** SysConfig warns that the action qualifier and MEP edge
   settings of ePWM1 (up count, both edges) and ePWM4 (AQ on CMPA up/down with
   both-edge MEP) do not match the HRPWM rules in the TRM.
3. **ePWM4 is never synchronised:** phase load is enabled, but its sync input is
   EPWM1SYNCOUT and ePWM1 has sync out disabled.
4. **No trip zones** are configured on ePWM4 (`TZSEL` = 0), and the emulation
   mode is "stop after next time-base cycle": the outputs freeze when CPU1 is
   halted by the debugger.
