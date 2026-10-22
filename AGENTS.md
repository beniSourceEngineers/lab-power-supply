# AGENTS.md — Lab Power Supply (TMS320F28379D)

This file gives humans and coding agents the overview, goals, constraints and
plan for this repository. Read it before changing anything.

---

## 1. Goal

A lab power supply whose firmware runs on the **TI TMS320F28379D** (dual C28x
cores CPU1/CPU2, each with its own CLA). Today the firmware is built **entirely
from Simulink** (MathWorks *C2000 Microcontroller Blockset* + Embedded Coder).

This repo shows **how to move from a fully generated workflow to a partly
generated one**:

| | Fully generated (today) | Target |
|---|---|---|
| Control algorithms | Simulink → generated C | Simulink → generated C (**unchanged, shared**) |
| Peripherals (ADC, ePWM, GPIO, SCI, …) | C2000 blockset blocks | Handwritten C++03 on top of C2000Ware driverlib |
| Scheduling / ISRs / state machine / glue logic | Simulink | Handwritten C++03 |
| Inter-core communication (IPC) | IPC blocks | C++03 IPC abstraction (this repo) |
| Build | Simulink "Build, Deploy & Start" | Code Composer Studio (CCS) projects |

When finished, the repo contains **two complete firmware projects that use the
same generated control code**:

1. `simulink_full/` — the reference, fully generated project.
2. `ccs/` — the Code Composer Studio project in C++03 that only uses generated
   code for the control part.

---

## 2. Repository layout (target)

```
lab-power-supply/
├── AGENTS.md                  ← this file
├── CLAUDE.md                  ← imports AGENTS.md
├── README.md
├── toolchain.md               ← pinned tool versions (see §4)
│
├── control/                   ← SHARED: single source of truth for control
│   ├── models/                ← control models only (.slx), no hardware blocks
│   │   ├── VoltageLoop.slx
│   │   ├── CurrentLoop.slx
│   │   └── ...
│   ├── data/                  ← Simulink data dictionary (.sldd): buses,
│   │                            parameters, shared configuration set
│   ├── scripts/
│   │   └── generate_control.m ← generates code for all control models
│   └── generated/             ← generated C code (committed, reviewed in PRs)
│       ├── VoltageLoop/
│       ├── CurrentLoop/
│       └── shared/            ← shared utils / rtwtypes.h
│
├── simulink_full/             ← PROJECT A: fully generated reference
│   ├── LabPsu_CPU1.slx        ← top model with C2000 blockset I/O, references
│   ├── LabPsu_CPU2.slx          control/models/* via Model blocks
│   └── scripts/
│
├── ccs/                       ← PROJECT B: Code Composer Studio, C++03
│   ├── common/                ← code shared by both cores
│   │   ├── ipc/               ← C++ IPC abstraction (§6)
│   │   ├── shared_types/      ← message structs exchanged between cores
│   │   └── util/
│   ├── cpu1/                  ← CCS project for CPU1: control, power stage
│   │   ├── src/               ← main, ISRs, HAL, application, control wrappers
│   │   ├── cla/               ← later: CLA tasks (C only, see §5)
│   │   └── cmd/               ← linker command files
│   ├── cpu2/                  ← CCS project for CPU2: communication
│   │   ├── src/               ← main, comm layer (SCI now, CANopen later)
│   │   ├── third_party/       ← licensed CANopen stack (see §3.5)
│   │   └── cmd/
│   └── ext/
│       └── c2000ware/         ← pinned copy of C2000Ware (driverlib, device
│                                support, linker files) for handwritten code only
│
├── test/
│   ├── host/                  ← host unit tests (IPC, glue logic) on PC
│   └── back_to_back/          ← model vs. generated code vs. target comparison
└── docs/
```

Rule: **nothing under `ccs/` or `simulink_full/` contains control algorithms.**
Control lives only in `control/models/`, and its code only in
`control/generated/`.

---

## 3. Key architectural decisions

### 3.1 Control models are "pure"

- Control models contain math only: no ADC, PWM, GPIO, IPC, interrupt or other
  C2000 blockset blocks.
- Inputs and outputs are **bus objects** defined in the data dictionary, e.g.
  `CurrentLoopIn` (measured values, setpoints, limits, enable) and
  `CurrentLoopOut` (duty cycle, saturation flags).
- All control is **floating point**: physical units as `single` (FPU32). No
  fixed point and no `double`.
- Each model has an explicit sample time and one entry point (`step`) plus
  `initialize`. Multi-rate models are avoided unless needed.

### 3.2 One code generation configuration, used by both projects

- All control models use a **configuration reference** to one shared
  configuration set in `control/data/*.sldd`. Its settings: ERT target, hardware
  board *TI Delfino F2837xD*, `single` as default floating type, no `double`,
  no dynamic memory, no non-finite support unless needed.
- `control/scripts/generate_control.m` runs a standalone ERT build per control
  model and copies the code to `control/generated/<Model>/`.
- **Project B (CCS)** compiles `control/generated/` directly.
- **Project A (Simulink full)** uses the same models through **Model
  reference blocks** (decided). Because both builds use the same configuration
  set, the generated model-reference code must match `control/generated/`.
  CI checks this (§8).
- Code interface: prefer **reusable (multi-instance) code** with the model data
  passed as an argument, so a C++ wrapper class can own each instance. Function
  prototypes are fixed with *Configure C Step Function Interface*, e.g.
  `void CurrentLoop_step(RT_MODEL_CurrentLoop *rtM, const CurrentLoopIn *in, CurrentLoopOut *out);`
- Generated code is committed so that diffs are reviewable and the CCS
  project builds without MATLAB.

### 3.3 Glue code is C++03 on top of C2000Ware driverlib

- C++ wraps the generated C through `extern "C"` headers and thin wrapper
  classes (`ctrl::CurrentLoop`, …) that map HAL data to model buses.
- HAL classes wrap driverlib (ADC, ePWM, CMPSS, GPIO, SCI/CAN, CPU timers),
  with no business logic.
- C2000Ware is **copied** into `ccs/ext/c2000ware/` and used **only by the
  handwritten code** (Project B). Project A uses the C2000Ware that the
  MathWorks C2000 Microcontroller Blockset installs and expects; do not point
  the blockset at the copy in this repo. Copy only what is needed (F2837xD
  driverlib, device headers/support, linker command files), and keep the
  license files.
- The application layer covers the state machine (Off / Soft-start / CV / CC /
  Fault), protection, setpoint handling, communication and HMI.

### 3.4 Core and CLA allocation (decided)

| Unit | Responsibilities |
|---|---|
| CPU1 | System init, boot of CPU2, peripheral ownership hand-over, ADC/ePWM/CMPSS HAL, **all control loops**, protection/trip, state machine |
| CLA1 | **Not used for now.** Later step: move selected loops here (§7, Phase 10) |
| CPU2 | **Communication**: SCI now, licensed CANopen stack later; command parsing, setpoints/limits to CPU1 via IPC, measurements/status from CPU1 |
| CLA2 | Not used |

Keep the control wrappers on CPU1 free of assumptions about the executing
unit (inputs/outputs only through the bus structs), so a later move to CLA1 does
not change the interface.

CPU1 must boot CPU2, assign GSx RAM and peripheral ownership (`MemCfg`,
`SysCtl_selectCPUForPeripheral`, e.g. the SCI and later CAN module to CPU2) and
synchronise with CPU2 at startup (`IPC_sync`).

### 3.5 Communication on CPU2

- **Now:** SCI (UART) with a simple framed protocol (e.g. SLIP/COBS + CRC16):
  set voltage/current/limits, output on/off, read measurements and status.
- **Later:** a licensed third-party **CANopen** stack. To prepare:
  - The application talks to an abstract `comm::` interface (object
    dictionary-like parameter access: read/write by index), not to SCI
    directly. SCI and CANopen become two implementations of it.
  - The stack goes into `ccs/cpu2/third_party/canopen/`. Check the license
    before committing it. If it may not be in this repo, reference it via an
    install-path variable and keep only the porting layer (driver/timer glue)
    here.
  - The stack will most likely be C. Integrate it via `extern "C"` and check
    its C2000 port (16-bit `char`, CAN driver on driverlib).

---

## 4. Toolchain (pin exact versions in `toolchain.md`)

- MATLAB/Simulink **R2025b** with Embedded Coder and C2000 Microcontroller Blockset
- Code Composer Studio (CCS)
- TI C2000 Code Generation Tools (C28x compiler, which also compiles CLA code)
- C2000Ware (driverlib, device support, linker command files): copied into
  `ccs/ext/c2000ware/` for Project B only. **Proposed: 5.02.00.00**, the
  version MathWorks verifies for the R2025b blockset, so both projects use the
  same version. Not yet confirmed; nothing is copied before confirmation.
- C28x compiler 22.6.0.LTS and CCS 20.0.1, both verified for R2025b
- Licensed CANopen stack (later), version to be recorded when it arrives
- Hardware: LAUNCHXL-F28379D or the custom power stage board

Every release of either project must state which versions built it.

---

## 5. Coding rules

### C++ (CCS project)

- **C++03 only** (C28x compiler limit). Do not use `auto`, `nullptr`,
  `constexpr`, `static_assert`, lambdas, `override`, `enum class`, `<cstdint>`,
  `<atomic>` or variadic templates.
  - Use `#include <stdint.h>`, a `STATIC_ASSERT` macro (negative-array trick),
    and `enum` inside a `struct` or `namespace` for scoping.
- No exceptions, no RTTI, **no heap** (`new`/`malloc` are forbidden after init;
  preferably never). All objects are static.
- Avoid virtual calls in ISRs; use templates/static polymorphism in hot paths.
- **`char` is 16 bit** (`CHAR_BIT == 16`), and `sizeof` counts 16-bit words.
  Never assume byte layout, and there is no real 8-bit `uint8_t`. Use
  `uint16_t`/`uint32_t` in all shared and IPC structures.
- Use `volatile` for memory shared between cores or CLA and CPU. 32-bit accesses
  are atomic on C28x, but larger structures need a protocol (§6).
- Section placement uses `#pragma DATA_SECTION("sec")` / `#pragma CODE_SECTION("sec")`,
  written immediately before the declaration (C++ form).
- Time-critical functions run from RAM (`.TI.ramfunc`), copied at boot.
- Naming: namespaces `hal::`, `ipc::`, `ctrl::`, `app::`. Files are
  `snake_case.hpp/.cpp`, and types are `PascalCase`.

### CLA (later step, not used now)

- The CLA compiler supports **C only**, so no C++ is allowed in `cla/`.
- CLA restrictions: no recursion, no function pointers, limited integer
  division/modulo, `float` only (no `double`), no standard library.
- Generated control code that runs on the CLA must meet these restrictions.
  Check the code-generation options (Phase 3) and build the generated files with
  the CLA compiler (`.cla` extension or `--cla_support`).

### Simulink

- Control models live only in `control/models/`, with all data in the data
  dictionary.
- Never edit files in `control/generated/` by hand. Regenerate them with the
  script.

---

## 6. IPC abstraction (C++03)

F28379D IPC resources are 32 IPC flags per direction (with interrupts on
flags 0–3), the message RAMs `CPU1TOCPU2MSGRAM`/`CPU2TOCPU1MSGRAM`
(write-only for the sender), and GSx RAM with configurable owner.

Planned design (`ccs/common/ipc/`):

```cpp
namespace ipc {

// Fixed-size typed channel in a message RAM section, one writer / one reader.
// Consistency through a sequence counter (seqlock) or double buffer.
template <typename T, uint16_t Flag>
class Mailbox {
public:
    void post(const T& msg);          // writer core: copy + seq++ + set flag
    bool tryRead(T& out);             // reader core: consistent copy if new
    bool hasNew() const;
};

// Queue of fixed-size elements (ring buffer) for events/commands.
template <typename T, uint16_t Size, uint16_t Flag>
class Queue { /* push / pop, single producer / single consumer */ };

// Command with acknowledge, built on driverlib IPC_sendCommand/IPC_waitForAck.
class CommandLink { /* send(cmd, addr, data), poll()/ISR dispatch */ };

void syncCores(uint16_t flag);        // boot-time handshake (IPC_sync)

} // namespace ipc
```

- Message types live in `ccs/common/shared_types/`. Both cores include the same
  header so the layout is identical, and every type has a
  `STATIC_ASSERT(sizeof(T) == N)` check.
- Section names and addresses are defined once, and both linker command files
  use them.
- Unit tests for the protocols (seqlock, ring buffer) run on the host in
  `test/host/`, with the register access replaced.

---

## 7. Plan / milestones

Each phase ends with one PR. Each phase must keep both projects buildable.

| # | Phase | Deliverable / exit criterion |
|---|---|---|
| 0 | **Repo setup** | Layout, `toolchain.md`, `.gitignore` (slprj, codegen, CCS Debug/Release), this file |
| 1 | **Baseline import** | Existing fully generated model in `simulink_full/`, builds and runs on target; reference measurements recorded (step response, ISR timing, CPU load) |
| 2 | **Partition the model** | Control algorithms extracted to `control/models/` with bus interfaces and data dictionary; `simulink_full/` references them; behaviour equal to Phase 1 (SIL/PIL back-to-back + target measurement) |
| 3 | **Shared code generation** | `generate_control.m` gives deterministic code in `control/generated/`; interface (prototypes, memory sections) defined; Project A model-reference code equals `control/generated/` |
| 4 | **CCS skeleton** | `ccs/cpu1`, `ccs/cpu2` projects in C++03 with driverlib, linker files, boot of CPU2, blinky + timer ISR on both cores |
| 5 | **IPC library** | `ipc::Mailbox`, `Queue`, `CommandLink`, `syncCores`; host unit tests; target test with both cores exchanging data |
| 6 | **HAL + glue logic** | ADC/ePWM/CMPSS/trip-zone HAL, state machine, protection; open loop on hardware |
| 7 | **Integrate control** | Generated control called from ISR/CLA through C++ wrappers; closed loop on hardware |
| 8 | **Verification** | Back-to-back comparison Project A and Project B (same inputs give the same outputs); timing/CPU load compared with Phase 1 |
| 9 | **Documentation** | Migration guide in `docs/`: steps, pitfalls, comparison of the workflows |
| 10 | **CLA (later)** | Selected control loops moved to CLA1 in both projects; CLA-compatible code generation; back-to-back again |
| 11 | **CANopen (later)** | Licensed CANopen stack on CPU2 behind the `comm::` interface; SCI stays as service/debug interface |

---

## 8. Verification and CI (target state)

- **Generated code check**: regenerate `control/generated/` in MATLAB batch mode
  and fail if `git diff` is non-empty.
- **Model reference equality**: the control code that Project A generates
  matches `control/generated/`.
- **CCS headless build** of `cpu1` and `cpu2` (`eclipsec -noSplash -application com.ccstudio.apps.projectBuild ...`)
  with warnings treated as errors for handwritten code.
- **Host unit tests** for `ccs/common/` (IPC protocol, utilities).
- **Back-to-back**: recorded input vectors run through the model (MIL), the
  generated code (SIL) and the target (PIL/HIL), and the outputs are compared
  within a tolerance.

---

## 9. Decisions and open questions

Decided (2026-10-22):

- All control loops run on CPU1. Floating point (`single`).
- No CLA for now; CLA comes in a later step (Phase 10).
- Project A uses model reference for the control models.
- C2000Ware is copied into the repo, for handwritten code only.
- Communication runs on CPU2: SCI now, licensed CANopen stack later.
- MATLAB R2025b Update 5 (see `toolchain.md`).

Open:

1. C2000Ware 5.02.00.00 / C28x compiler 22.6.0.LTS / CCS 20.0.1: proposed, awaiting confirmation
   (see `toolchain.md`).
2. Simulink models: to be supplied, then Phase 1 starts.
3. CANopen stack: vendor, license terms (may it be committed?), C2000 port.

---

## 10. Working rules for agents

- Work on a **new branch** for every change (`<type>/<short-topic>`, e.g.
  `feat/ipc-mailbox`). Never commit directly to `main`.
- Commit in small, self-contained steps. Open a PR when the task is done.
- Do **not** add AI/tool attribution (no `Co-Authored-By`, no "Generated
  with …") to commits or PRs.
- Do not edit `control/generated/` by hand.
- Do not introduce C++11 or later features into `ccs/` (§5).
- Treat `.slx`/`.sldd` files as binary. Describe model changes in the PR text.
