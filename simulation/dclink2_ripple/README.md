# DC link 2 ripple: PI controller 2 with resonant terms

Standalone study, independent of `control/` and the other simulation models.

## System

Single-phase PFC → DC link 1 (400 V, **not measured**) → buck (L = 220 µH,
C = 40 µF) → DC link 2 → full bridge → cascade. PI controller 1 sets
`VbuckSet`. PI controller 2 controls DC link 2 and outputs the modulation index `m`.
Its control rate is 9.6667 kHz, with one sample of computation delay.

The DC link 1 ripple comes from a PFC that is not well synchronised to the
grid. It contains harmonics of 50 Hz, with 50 Hz dominant at about 7 V, and
their amplitude and phase change with the operating point.
`dclink1_ripple.m` models this. Its spectrum is **assumed** and should be
replaced by a measured spectrum.

## Design (`design_dclink2_ripple.m`)

| Part | Value | Why |
|---|---|---|
| Integral | Ki = 0.3 /(V s), Kp = 0 | The LC resonance (1.7 kHz) is nearly undamped at light load and only 5.7× below the control rate. A P part or a faster integral destabilises it. |
| Resonant terms | 50, 100, …, 300 Hz, Kr = 0.5, 0.25, 0.2, 0.15, 0.12, 0.1 /V, ±0.5 Hz wide | Infinite-gain-like rejection at each harmonic. Kr falls with the order to limit the sensitivity peak. |
| Phase compensation | θ_h = −∠(notch · plant · delay) at h·50 Hz | Keeps the loop stable with six resonances. |
| LC notch on the error | 1.6 kHz, ζ 0.15 / 0.7 | Without it the resonant terms raise the sensitivity peak to 19 dB at no load. |

Worst case over 30 W … 3 kW, Vdc1 ±2 % and L, C ±10 %:

| Controller | Ms | \|S\| at 50 / 100 / 150 / 200 / 250 / 300 Hz | \|S\| at 350 … 500 Hz |
|---|---|---|---|
| Previous (50 Hz only, no notch) | 9.4 dB | −30 / −1 / 0 / 0 / 0 / 0 dB | 0 dB |
| New | 5.7 dB | −30 / −24 / −22 / −20 / −18 / −17 dB | +0.8 … +1.3 dB |

The results are the same with coefficients rounded to single precision (C28x FPU32).

## Simulation (`compare_dclink2_ripple.m`)

DC link 2 ripple, peak to peak:

| Operating point | PI only | Previous | New |
|---|---|---|---|
| 3 kW | 15.3 V | 8.7 V | 2.3 V |
| 900 W | 7.7 V | 3.9 V | 1.2 V |
| 300 W | 5.6 V | 2.7 V | 0.8 V |
| 30 W | 4.6 V | 2.1 V | 0.7 V |
| 3 kW, grid 49.8 Hz | 15.3 V | 8.7 V | 2.8 V |

What is left with the new controller comes mostly from the harmonics above
300 Hz (350 … 500 Hz), which have no resonant term.

## Firmware (per control step, single precision)

```
e  = VbuckSet - vdc2
en = notch(e)                       // biquad, DF2T, coefficients below
I  = clamp(I + Ki*Ts*en, mMin, mMax) // integral with clamping
r  = sum_h biquad_h(en)             // resonant terms, DF2T each
m  = clamp(I + r, mMin, mMax)

biquad (DF2T): y = b0*x + s1;  s1 = b1*x - a1*y + s2;  s2 = b2*x - a2*y
```

`design_dclink2_ripple` prints the coefficients. Run the resonant terms in
DF2T or as separate biquads, not as one high-order filter: their poles are at
|z| = 0.99967.

## Open points

- The DC link 1 spectrum is assumed. Measure it at a few operating points,
  then choose `resH` and `resKr` from it.
- The 30 Ω load, the 50 mΩ series resistance and the ±10 % L and C tolerances
  are assumptions.
- Load step 300 W → 3 kW: DC link 2 drops by about 20 V with every
  controller. The slow integral (limited by the undamped LC) sets this, not
  the resonant terms. A faster response needs damping of the LC (inner
  current loop or capacitor current feedback) or load-current feedforward.
- The resonant states have no anti-windup. In normal operation `m` stays
  well inside 0 … 0.95.

## Files

| File | Content |
|---|---|
| `dclink2_ripple_params.m` | Parameters (data source of the model workspace) |
| `dclink1_ripple.m` | DC link 1 ripple spectrum per operating point (assumed) |
| `dclink2_resonant_coeffs.m`, `dclink2_notch_coeffs.m` | Discrete controller coefficients |
| `build_dclink2_ripple.m` | Creates `dclink2_ripple.slx` |
| `design_dclink2_ripple.m` | Margins over operating points and tolerances (Control System Toolbox) |
| `compare_dclink2_ripple.m` | Simulations: PI only vs previous vs new |
