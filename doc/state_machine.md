# State machine — interface for the Stateflow model (draft v0.1)

Status: **draft for review**. Defines what `control/models/StateMachine.slx`
must provide so that its generated code plugs into Project B as variant **B1**
(AGENTS.md §3.7), and what the handwritten variant **B2** (Phase 6a) must
reproduce.

The state machine is used **only through the code generated from the
Stateflow model**. Project B does not re-implement it; the C++ side is a thin
wrapper (`app::StateflowStateMachine`, added together with the generated code)
behind `app::IStateMachine` (`ccs/cpu1/src/app/IStateMachine.hpp`).

## 1. Findings in the existing models

Checked 2026-11-11 in `simulink_full/LabPSU_CPU01.slx` and `LabPSU_CPU02.slx`:

- There is **no Stateflow state machine**. The only Stateflow block (CPU02,
  `Subsystem1/checkForBusChanges`) is a MATLAB Function block.
- There is **no protection logic** in `Current_Control` / `Voltage_Control`,
  and no CMPSS or trip-zone configuration in the ePWM blocks.

So the state machine (and the protection) are new models, not extractions.

## 2. Model set-up

Same rules as the control models (AGENTS.md §3.1, §3.2):

| Item | Value |
|---|---|
| Model | `control/models/StateMachine.slx`, Stateflow chart, no hardware blocks |
| Configuration | reference to the shared configuration set in `control/data/configuration.sldd` |
| Data | `control/data/StateMachine.sldd` (buses, enum), referencing `configuration.sldd` |
| Sample time | `1e-4` s (10 kHz, same ISR as the control loops) |
| Code interface | reusable (multi-instance), like `Current_Control` |
| Entry points | `StateMachine_initialize(rtM)`, `StateMachine_step(rtM, U, Y)` |
| Generated to | `control/generated/StateMachine/` by `generate_control.m` |

## 3. Interface

### Input bus `StateMachine_in`

| Element | Type | Meaning |
|---|---|---|
| `outputEnableRequest` | `boolean` | Output on/off command from CPU2 |
| `faultResetRequest` | `boolean` | Acknowledge a fault |
| `faultActive` | `boolean` | Protection or hardware trip active |
| `voltageAtSetpoint` | `boolean` | Voltage loop: ramped setpoint reached the reference (`VoltageControl_out.isAtSetpoint`) |
| `currentLimitActive` | `boolean` | The current limit determines the output |

### Output bus `StateMachine_out`

| Element | Type | Meaning |
|---|---|---|
| `state` | `LabPsuState` | Active state |
| `powerStageEnable` | `boolean` | Release the PWM outputs (clear the trip) |
| `controlEnable` | `boolean` | `enable` input of the voltage and current loop |

### Enumeration `LabPsuState`

`Simulink.IntEnumType`, storage type `uint16`, default `Off`, header
generated with the model. The values must match `app::State::Type`:

| Name | Value |
|---|---|
| `Off` | 0 |
| `SoftStart` | 1 |
| `ConstantVoltage` | 2 |
| `ConstantCurrent` | 3 |
| `Fault` | 4 |

The wrapper maps the buses to `app::StateMachineInputs` /
`app::StateMachineOutputs` field by field, so names and types may still change
during review; the table above is what the wrapper will be written against.

## 4. Behaviour (proposal)

Initial state `Off`. In every state, a transition to `Fault` has the highest
priority.

| State | `powerStageEnable` | `controlEnable` | Transitions (in priority order) |
|---|---|---|---|
| `Off` | 0 | 0 | `faultActive` → `Fault`; `outputEnableRequest` → `SoftStart` |
| `SoftStart` | 1 | 1 | `faultActive` → `Fault`; `!outputEnableRequest` → `Off`; `currentLimitActive` → `ConstantCurrent`; `voltageAtSetpoint` → `ConstantVoltage` |
| `ConstantVoltage` | 1 | 1 | `faultActive` → `Fault`; `!outputEnableRequest` → `Off`; `currentLimitActive` → `ConstantCurrent` |
| `ConstantCurrent` | 1 | 1 | `faultActive` → `Fault`; `!outputEnableRequest` → `Off`; `!currentLimitActive` → `ConstantVoltage` |
| `Fault` | 0 | 0 | `faultResetRequest && !faultActive` → `Off` |

**[OPEN]** Soft start: is the voltage ramp of `Voltage_Control` (`slewrate`,
`rampedSetpoint`) the soft start, or does the state machine need its own
timing (e.g. a minimum time in `SoftStart`)?

## 5. Tests

The Simulink Test scenarios for the state machine (`simulation/tests/`) record
input and output vectors. They are the reference for:

- B1: the generated code on the host (SIL) and on the target,
- B2: the handwritten C++ FSM, replayed in `test/integration/` (Phase 6a).
