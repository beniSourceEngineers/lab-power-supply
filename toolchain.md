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
| Code Composer Studio | 20.0.1 (blockset supports CCSv3 to CCSv20) | Project A and B |
| C2000Ware | 5.02.00.00 | Project A (blockset install), Project B (copy in `ccs/ext/c2000ware/`) |
| TI C28x Code Generation Tools | 22.6.0.LTS | Project A and B |

Status: **proposed, awaiting confirmation.** Nothing from C2000Ware has been
copied yet.

Reason: if both projects use the same C2000Ware and compiler, any difference in
the back-to-back comparison (Phase 8) comes from the workflow, not from the
tools. A newer C2000Ware (latest: 26.02.00.00 LTS with compiler 25.11.1.LTS) is
possible for Project B, but MathWorks has not verified it for R2025b.

## Third party

| Component | Version | Notes |
|---|---|---|
| CANopen stack | tbd | Licensed, later phase (see AGENTS.md §3.5) |
