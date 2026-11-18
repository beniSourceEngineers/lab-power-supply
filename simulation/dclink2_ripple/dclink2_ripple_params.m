%DCLINK2_RIPPLE_PARAMS Parameters of the standalone DC link 2 ripple model.
%
%   Data source of the model workspace of dclink2_ripple.slx. Standalone:
%   independent of control/ and of the other simulation models.
%
%   System: single-phase PFC -> DC link 1 (400 V, not measured, ripple with
%   harmonics of 50 Hz, see dclink1_ripple.m) -> buck (L, C) -> DC link 2 ->
%   full bridge. PI controller 2 controls DC link 2 to VbuckSet and outputs
%   the modulation index m. Goal: remove the ripple harmonics from DC link 2
%   with PI controller 2 -> PI plus resonant terms at the main harmonics.
%
%   Overridable per simulation (Simulink.SimulationInput.setVariable):
%   Rload1, Rload2, tStep, fg, resEnable, resGainScale.

%% Power stage
L      = 220e-6;        % H, buck inductor
C      = 40e-6;         % F, DC link 2 capacitor
RL     = 0.1;           % ohm, inductor + switch resistance (Beni 2026-11-17)
Rnom   = 30;            % ohm, nominal load on DC link 2 (ASSUMED: 3 kW at 300 V)

%% Control
fCtrl    = 9.6667e3;    % Hz, control frequency
Ts       = 1/fCtrl;     % s
VbuckSet = 300;         % V, DC link 2 setpoint (output of PI controller 1, constant here)
mMax     = 0.95;        % modulation limit
mMin     = 0;

%% Operating point and load step: load Rload1 until tStep, Rload2 after
Rload1 = Rnom;
Rload2 = Rnom;
tStep  = 0.6;           % s

%% DC link 1: nominal voltage and ripple (harmonics of the grid frequency)
Vdc1 = 400;             % V
fg   = 50;              % Hz, actual grid frequency (the resonant terms stay tuned to fRes)
Pnom = VbuckSet^2/Rnom;
[ripAmp1, ripPh1, ripH] = dclink1_ripple(VbuckSet^2/Rload1, Pnom);
[ripAmp2, ripPh2]       = dclink1_ripple(VbuckSet^2/Rload2, Pnom);

%% PI controller 2
% The LC filter resonates at 1/(2*pi*sqrt(L*C)) = 1.7 kHz, only 5.7x below
% fCtrl. RL = 100 mOhm damps it to zeta = 0.02 at no load (peak 28 dB above
% the DC gain), which alone does not allow a P part: Kp = 5e-5 is already
% unstable at no load. The low-pass below takes the loop gain away from the
% resonance and makes Kp = 1e-3 and a 7x faster integral possible
% (design_dclink2_ripple.m).
Kp = 1e-3;              % 1/V
Ki = 2;                 % 1/(V s)

%% Low-pass on the control error (in front of P, I and the resonant terms)
%   F(s) = wf^2 / (s^2 + 2*zeta*wf*s + wf^2), Tustin prewarped at wf
filtF    = 500;         % Hz
filtZeta = 0.7;
[filtNum, filtDen] = dclink2_lowpass_coeffs(filtF, filtZeta, Ts);

%% Resonant terms at the harmonics of fRes, in parallel with the PI
%   R_h(s) = Kr_h * (s*cos(th_h) - w_h*sin(th_h)) / (s^2 + 2*wc*s + w_h^2)
% Gain Kr_h/(2*wc) at w_h. th_h compensates the phase of low-pass + plant
% + delay at w_h (nominal load), which keeps the loop stable with many
% resonances.
% Each term: Tustin, prewarped at w_h (peak stays exactly at h*fRes).
fRes  = 50;                                   % Hz
resH  = [1 2 3 4 5 6];                        % harmonics with a resonant term
resKr = [0.5 0.25 0.2 0.15 0.12 0.1];         % 1/V, see design_dclink2_ripple.m
wc    = 2*pi*0.5;                             % rad/s, half bandwidth (+-0.5 Hz at each harmonic)
resEnable    = 1;                             % 1: PI + resonant terms, 0: PI only
resGainScale = 1;                             % multiplies all Kr (design checks)

[resA, resB, resC, resD, resBiquads] = dclink2_resonant_coeffs( ...
    resH*fRes, resKr, wc, Ts, L, C, RL, Rnom, Vdc1, filtNum, filtDen);

%% Operating point (start in steady state at Rload1)
iL0 = VbuckSet/Rload1;
vC0 = VbuckSet;
m0  = (VbuckSet + RL*iL0)/Vdc1;
