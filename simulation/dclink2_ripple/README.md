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

RL = 100 mΩ damps the LC resonance (1.7 kHz, 5.7× below the control rate) to
only ζ ≈ 0.02 at no load. The plant still peaks 26 dB above its DC gain, so a
P part needs a filter that takes the loop gain away from the resonance.

Two controllers are kept. Both are stable over the whole operating range.

| Part | New (default in `dclink2_ripple_params.m`) | Previous (`dclink2_previous_controller.m`) |
|---|---|---|
| P, I | Kp = 1e-3 /V, Ki = 2 /(V s) | Kp = 0, Ki = 0.3 /(V s) |
| Filter on the error | 2nd-order low-pass, 500 Hz, ζ 0.7 | Notch 1.6 kHz, ζ 0.15 / 0.7 |
| Resonant terms | 50 … 300 Hz, Kr = 0.5, 0.25, 0.2, 0.15, 0.12, 0.1 /V, ±0.5 Hz | same |
| Phase compensation | θ_h = −∠(filter · plant · delay) at h·50 Hz | same |

Worst case over 30 W … 3 kW, Vdc1 ±2 % and L, C ±10 % (`design_dclink2_ripple.m`,
Bode plots: `bode_dclink2_ripple.m`):

| Controller | Ms | GM / PM | Crossover without resonant terms | \|S\| at 10 Hz | \|S\| at 50 … 300 Hz | \|S\| at 350 … 500 Hz |
|---|---|---|---|---|---|---|
| New | 5.2 dB | 8.1 dB / 60° | 142 Hz | −22 dB | −30 … −16 dB | +2 … +5 dB |
| Previous | 2.6 dB | 16 dB / 81° | 19 Hz | −6 dB | −30 … −17 dB | +1 dB |
| New without low-pass | unstable | | | | | |

The results are the same with coefficients rounded to single precision (C28x FPU32).

## Simulation (`compare_dclink2_ripple.m`, RL = 100 mΩ)

DC link 2 ripple, peak to peak:

| Operating point | PI only (new Kp, Ki) | Previous | New |
|---|---|---|---|
| 3 kW | 10.1 V | 2.3 V | 3.3 V |
| 900 W | 4.7 V | 1.2 V | 1.7 V |
| 300 W | 3.3 V | 0.8 V | 1.2 V |
| 30 W | 2.6 V | 0.7 V | 1.0 V |
| 3 kW, grid 49.8 Hz | 10.0 V | 2.8 V | 3.7 V |
| Load step 300 W → 3 kW, max. deviation | 22.9 V | 23.9 V | 23.7 V |

Both controllers remove harmonics 1 … 6 equally well. The new one is faster
below 30 Hz, but it amplifies harmonics 7 … 10 (350 … 500 Hz) more, so its
total ripple is about 40 % higher. The load-step dip is the same for all
controllers: it is LC ringing in the first millisecond (3 kW into 40 µF),
which no voltage loop at this bandwidth can catch.

## Firmware (per control step, single precision)

```
e  = VbuckSet - vdc2
en = filter(e)                      // low-pass (new) or notch (previous), biquad DF2T
I  = clamp(I + Ki*Ts*en, mMin, mMax) // integral with clamping
r  = sum_h biquad_h(en)             // resonant terms, DF2T each
m  = clamp(Kp*en + I + r, mMin, mMax)

biquad (DF2T): y = b0*x + s1;  s1 = b1*x - a1*y + s2;  s2 = b2*x - a2*y
```

`design_dclink2_ripple` prints the coefficients. Run the resonant terms in
DF2T or as separate biquads, not as one high-order filter: their poles are at
|z| = 0.99967.

## Open points

- The DC link 1 spectrum is assumed. Measure it at a few operating points,
  then choose `resH` and `resKr` from it.
- The 30 Ω load and the ±10 % L and C tolerances are assumptions. RL = 100 mΩ
  is Beni's value.
- Load step 300 W → 3 kW: DC link 2 drops by about 23 V with every
  controller, set by the undamped LC. Reducing it needs damping of the LC
  (inner current loop or capacitor current feedback) or load-current
  feedforward.
- The resonant states have no anti-windup. In normal operation `m` stays
  well inside 0 … 0.95.

## Files

| File | Content |
|---|---|
| `dclink2_ripple_params.m` | Parameters (data source of the model workspace) |
| `dclink1_ripple.m` | DC link 1 ripple spectrum per operating point (assumed) |
| `dclink2_resonant_coeffs.m`, `dclink2_lowpass_coeffs.m` | Discrete controller coefficients |
| `dclink2_previous_controller.m` | Previous controller (Ki 0.3, notch) for comparison |
| `build_dclink2_ripple.m` | Creates `dclink2_ripple.slx` |
| `design_dclink2_ripple.m` | Margins over operating points and tolerances (Control System Toolbox) |
| `bode_dclink2_ripple.m` | Bode plots: plant, controller, open loop, S/T, ripple transfer |
| `compare_dclink2_ripple.m` | Simulations: PI only vs previous vs new |
