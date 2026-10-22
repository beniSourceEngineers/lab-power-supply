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
| Scheduling / ISRs / glue logic | Simulink | Handwritten C++03 |
| State machine | Stateflow → generated C | Two variants: generated Stateflow **or** handwritten C++03 FSM (§3.7) |
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
│   │   ├── CurrentLoop.slx    ← incl. protection logic (runs at ~10 kHz)
│   │   ├── StateMachine.slx   ← Stateflow state machine (§3.7)
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
├── simulation/                ← system simulation (MIL), not for code generation
│   ├── LabPsu_System.slx      ← control models (Model reference) + plant
│   ├── plant/                 ← power stage, output filter, load (Simscape)
│   ├── interface/             ← ADC/PWM/IPC effects between control and plant
│   └── tests/                 ← Simulink Test harnesses and test vectors
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
│   ├── mocks/                 ← one mock per interface: MockSensor for ISensor, …
│   ├── unit/                  ← host unit tests, one class under test, all
│   │                            dependencies mocked
│   ├── integration/           ← host integration tests: several real classes
│   │                            together, hardware mocked at the HAL interfaces
│   ├── target/                ← on-target integration tests (CCS test projects)
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
  Fault; two variants, §3.7), setpoint handling, communication and HMI.
- **Protection logic is part of the control models**, not the glue logic. It
  must run at the same rate as the control loop (~10 kHz), so it is generated,
  simulated and shared like the rest of the control. Hardware trips (CMPSS,
  trip zone) are configured by the HAL as a fast backstop.

### 3.4 Core and CLA allocation (decided)

| Unit | Responsibilities |
|---|---|
| CPU1 | System init, boot of CPU2, peripheral ownership hand-over, ADC/ePWM/CMPSS HAL, **all control loops incl. protection (~10 kHz)**, hardware trip configuration, state machine |
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

### 3.6 System simulation

A Simulink system model connects **all control models with plant models**
for model-in-the-loop (MIL) simulation of the whole power supply.

- `simulation/LabPsu_System.slx` references the control models in
  `control/models/` via **Model reference**, the same way Project A does.
  Simulation, Project A and Project B therefore all use the same control.
- Plant models (power stage, output filter, load, measurement circuits) live
  in `simulation/plant/` and use Simscape / Simscape Electrical. They never go
  into `control/`, so code generation never depends on Simscape.
- The effects of the hardware sit between the control and the plant, in
  `simulation/interface/`, not in the control models: ADC sampling and
  quantisation, PWM update delay, and the delay of setpoints from CPU2 to
  CPU1 over IPC.
- Simulink Test harnesses in `simulation/tests/` define the scenarios (start-up,
  load steps, CV/CC transitions, short circuit, ...). The recorded inputs and
  outputs are the reference vectors for the back-to-back comparison (§8).
- Protection is part of the control models, and the Stateflow state machine
  is a referenced model too, so the system simulation covers control,
  protection and state machine.

### 3.7 State machine: two variants

The state machine is modelled in **Stateflow** (`control/models/StateMachine.slx`).
Project B explores two variants and compares them:

| Variant | State machine | Built from |
|---|---|---|
| **B1** | Generated from the Stateflow model | `control/generated/StateMachine/`, same as Project A |
| **B2** | Handwritten C++03 FSM framework | `ccs/cpu1/src/app/fsm/` |

- Both variants implement the same interface `app::IStateMachine` (events in,
  state and output commands out). The composition root (`main.cpp`) selects
  the variant with a build configuration (CCS build configurations
  `StateflowFsm` / `CppFsm`). The rest of the application does not change.
- The Stateflow model is the **reference** for variant B2: the state machine
  scenarios from `simulation/tests/` are replayed on the C++ FSM in host
  integration tests (`test/integration/`), and the state sequence and outputs
  must match.
- The C++ FSM framework follows the coding rules (§5): C++03, no heap,
  states and transitions as static objects, interfaces and mocks for its
  dependencies.
- The migration guide (Phase 9) compares both variants: readability, testing,
  code size, execution time, and the effort to change behaviour.

---

## 4. Toolchain (pin exact versions in `toolchain.md`)

- MATLAB/Simulink **R2025b** with Embedded Coder and C2000 Microcontroller Blockset
- **Code Composer Studio 20.0.1** only (Theia-based CCS 20). Older CCS
  versions (Eclipse-based v12 and earlier) are not supported. Projects,
  `.projectspec` files and build scripts target CCS 20.
- **TI C28x compiler 22.6.0.LTS** (also compiles CLA code)
- **C2000Ware 5.02.00.00** (driverlib, device support, linker command files),
  copied into `ccs/ext/c2000ware/` for Project B only. Project A uses the same
  version from the blockset installation.
- These are the versions MathWorks verifies for the R2025b C2000 blockset, so
  both projects build with identical TI tools.
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
- Virtual calls are allowed (see *Interfaces* below). In the fast control ISR,
  keep the number of virtual calls per cycle small and measure the ISR time.
  Use templates only where a measurement shows that a virtual call is too slow.
- **`char` is 16 bit** (`CHAR_BIT == 16`), and `sizeof` counts 16-bit words.
  Never assume byte layout, and there is no real 8-bit `uint8_t`. Use
  `uint16_t`/`uint32_t` in all shared and IPC structures.
- Use `volatile` for memory shared between cores or CLA and CPU. 32-bit accesses
  are atomic on C28x, but larger structures need a protocol (§6).
- Section placement uses `#pragma DATA_SECTION("sec")` / `#pragma CODE_SECTION("sec")`,
  written immediately before the declaration (C++ form).
- Time-critical functions run from RAM (`.TI.ramfunc`), copied at boot.
- Naming: namespaces `hal::`, `ipc::`, `ctrl::`, `app::`, `comm::`. Types are
  `PascalCase`, and each file has the name of the class it contains
  (`Sensor.hpp`/`Sensor.cpp`).

### Interfaces

Classes that other classes depend on are reached through an **interface**:
a pure abstract class whose name starts with `I`.

| File | Contents |
|---|---|
| `ISensor.hpp` | Interface `ISensor`: pure virtual methods only, no data |
| `Sensor.hpp` / `Sensor.cpp` | Implementation `class Sensor : public ISensor` |
| `test/mocks/MockSensor.hpp` | Mock `class MockSensor : public ISensor` |

A pure interface has no code, so it only needs a header. Add an `ISensor.cpp`
only if the interface ever gets code.

```cpp
// ISensor.hpp
#ifndef HAL_ISENSOR_HPP
#define HAL_ISENSOR_HPP

namespace hal {

class ISensor {
public:
    virtual float read() const = 0;
    virtual bool  isValid() const = 0;

protected:
    // Protected, non-virtual: objects are never deleted through the interface
    // (no heap), so the compiler does not generate deleting destructors and
    // operator delete is not linked.
    ~ISensor() {}
};

} // namespace hal

#endif
```

Rules:

- Depend on interfaces, not implementations. Dependencies are passed as
  **references in the constructor** (dependency injection). There are no
  singletons and no global access from inside classes.
- Only the composition root (`main.cpp` of each core) knows the concrete
  classes. It creates them as static objects and wires them together.
- Interfaces live next to their implementation in the same namespace. Shared
  interfaces (IPC, shared types) live in `ccs/common/`.
- Generated control code is wrapped by a class behind an interface too
  (`ctrl::ICurrentLoop` → `ctrl::CurrentLoop`, which calls the generated C).
  This lets the application be tested with a mocked controller.
- Not every class needs an interface. Value types, message structs and
  small helpers without hardware or dependencies do not.

### Tests

- **Every interface has a mock** in `test/mocks/` (`MockSensor` for `ISensor`).
  Unit and integration tests use these mocks for every dependency they do not
  test.
- **Unit tests** (`test/unit/`): one class under test, all dependencies mocked.
  File name `<Class>Test.cpp`, e.g. `SensorTest.cpp`.
- **Integration tests** (`test/integration/`): several real classes wired
  together (e.g. state machine (B1 or B2) + controller wrapper + generated
  control code incl. protection). Hardware is mocked at the HAL interfaces (`IAdc`, `IPwm`, …).
- Host tests are built with CMake and run on the PC with **GoogleTest/GoogleMock**.
  GoogleTest needs a newer C++ standard, so:
  - only test code (`test/`) may use newer C++,
  - the production code is additionally compiled on the host with
    `-std=c++03 -pedantic` to catch C++11 features that slip in.
- **On-target integration tests** (`test/target/`): CCS test projects for
  driver, IPC and timing checks on the LaunchPad. Mocks used there must be
  hand-written C++03 (no GoogleMock).
- New or changed classes come with unit tests in the same PR.

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

// IMailboxWriter.hpp / IMailboxReader.hpp: what application code depends on.
template <typename T>
class IMailboxWriter {
public:
    virtual void post(const T& msg) = 0;   // copy + seq++ + set flag
protected:
    ~IMailboxWriter() {}
};

template <typename T>
class IMailboxReader {
public:
    virtual bool tryRead(T& out) = 0;      // consistent copy if new
    virtual bool hasNew() const = 0;
protected:
    ~IMailboxReader() {}
};

// Mailbox.hpp: fixed-size typed channel in a message RAM section, one writer /
// one reader. Consistency through a sequence counter (seqlock) or double buffer.
// Mocks: test/mocks/MockMailboxWriter.hpp, MockMailboxReader.hpp
template <typename T, uint16_t Flag>
class Mailbox : public IMailboxWriter<T>, public IMailboxReader<T> { /* ... */ };

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
- The IPC classes are reached through interfaces (`IMailboxWriter<T>`,
  `IMailboxReader<T>`, `IQueue<T>`, `ICommandLink`), so application code can be
  tested with mocks. Unit tests for the protocols
  (seqlock, ring buffer) run on the host in `test/unit/`, with the register
  access replaced.

---

## 7. Plan / milestones

Each phase ends with one PR. Each phase must keep both projects buildable.

| # | Phase | Deliverable / exit criterion |
|---|---|---|
| 0 | **Repo setup** | Layout, `toolchain.md`, `.gitignore` (slprj, codegen, CCS Debug/Release), this file |
| 1 | **Baseline import** | Existing fully generated model in `simulink_full/`, builds and runs on target; reference measurements recorded (step response, ISR timing, CPU load) |
| 2 | **Partition the model** | Control algorithms extracted to `control/models/` with bus interfaces and data dictionary; `simulink_full/` references them; behaviour equal to Phase 1 (SIL/PIL back-to-back + target measurement) |
| 2a | **System simulation** | `simulation/LabPsu_System.slx` with all control models + plant models; Simulink Test scenarios; reference vectors recorded |
| 3 | **Shared code generation** | `generate_control.m` gives deterministic code in `control/generated/`; interface (prototypes, memory sections) defined; Project A model-reference code equals `control/generated/` |
| 4 | **CCS skeleton** | `ccs/cpu1`, `ccs/cpu2` projects in C++03 with driverlib, linker files, boot of CPU2, blinky + timer ISR on both cores |
| 5 | **IPC library** | IPC interfaces + `ipc::Mailbox`, `Queue`, `CommandLink`, `syncCores`; mocks; host unit tests; target test with both cores exchanging data |
| 6 | **HAL + glue logic** | ADC/ePWM/CMPSS/trip-zone HAL, `app::IStateMachine` with variant B1 (generated Stateflow); open loop on hardware |
| 6a | **C++ FSM (variant B2)** | C++03 FSM framework + state machine; host tests replay the Stateflow scenarios; both variants selectable by build configuration |
| 7 | **Integrate control** | Generated control called from ISR/CLA through C++ wrappers; closed loop on hardware |
| 8 | **Verification** | Back-to-back comparison Project A and Project B, both FSM variants (same inputs give the same outputs); timing/CPU load compared with Phase 1 |
| 9 | **Documentation** | Migration guide in `docs/`: steps, pitfalls, comparison of the workflows |
| 10 | **CLA (later)** | Selected control loops moved to CLA1 in both projects; CLA-compatible code generation; back-to-back again |
| 11 | **CANopen (later)** | Licensed CANopen stack on CPU2 behind the `comm::` interface; SCI stays as service/debug interface |

---

## 8. Verification and CI (target state)

- **Generated code check**: regenerate `control/generated/` in MATLAB batch mode
  and fail if `git diff` is non-empty.
- **Model reference equality**: the control code that Project A generates
  matches `control/generated/`.
- **CCS headless build** of `cpu1` and `cpu2` with the CCS 20 command line
  (`ccs-server-cli -noSplash -workspace <ws> -application com.ti.ccs.apps.buildProject -ccs.projects <name>`)
  with warnings treated as errors for handwritten code.
- **Host unit and integration tests** (`test/unit/`, `test/integration/`) with
  GoogleTest/GoogleMock, plus a host build of the production code with
  `-std=c++03 -pedantic`.
- **Back-to-back**: input vectors recorded in the system simulation
  (`simulation/tests/`, MIL) run through the generated code (SIL) and the
  target (PIL/HIL), and the outputs are compared within a tolerance.

---

## 9. Decisions and open questions

Decided (2026-10-22):

- All control loops run on CPU1. Floating point (`single`).
- No CLA for now; CLA comes in a later step (Phase 10).
- Project A uses model reference for the control models.
- C2000Ware is copied into the repo, for handwritten code only.
- Communication runs on CPU2: SCI now, licensed CANopen stack later.
- Protection logic is part of the control models (~10 kHz).
- State machine in Stateflow; Project B explores generated Stateflow (B1) and
  a handwritten C++03 FSM (B2).
- MATLAB R2025b Update 5, CCS 20.0.1, C28x compiler 22.6.0.LTS, C2000Ware
  5.02.00.00 for both projects (see `toolchain.md`).

Open:

1. Simulink models: to be supplied, then Phase 1 starts.
2. CANopen stack: vendor, license terms (may it be committed?), C2000 port.
3. How the existing model is set up (protection inside the control models,
   Stateflow chart, rates): to be checked when the models are supplied.
4. Plant models: existing ones to be supplied, or built in Phase 2a?

---

## 10. Working rules for agents

- Work on a **new branch** for every change (`<type>/<short-topic>`, e.g.
  `feat/ipc-mailbox`). Never commit directly to `main`.
- Commit in small, self-contained steps. Open a PR when the task is done.
- Do **not** add AI/tool attribution (no `Co-Authored-By`, no "Generated
  with …") to commits or PRs.
- Do not edit `control/generated/` by hand.
- Do not introduce C++11 or later features into `ccs/` (§5).
- Use interfaces (`I<Name>`) for dependencies, and add a mock and unit tests for
  every new interface and class (§5).
- Treat `.slx`/`.sldd` files as binary. Describe model changes in the PR text.
