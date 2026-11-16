%DCLINK2_RIPPLE_PARAMS Parameters of the standalone DC link 2 ripple model.
%
%   Data source of the model workspace of dclink2_ripple.slx. Standalone:
%   independent of control/ and of the other simulation models.
%
%   System: PFC -> DC link 1 (400 V, not measured, 50 Hz ripple) -> buck
%   (L, C) -> DC link 2 -> full bridge. PI controller 2 controls DC link 2
%   to VbuckSet and outputs the modulation index m. Goal: remove the 50 Hz
%   component of DC link 2 with PI controller 2 -> PI plus a resonant term.

%% Power stage
L      = 220e-6;        % H, buck inductor
C      = 40e-6;         % F, DC link 2 capacitor
RL     = 0.05;          % ohm, inductor + switch resistance (assumed)
Rload  = 30;            % ohm, load seen on DC link 2 (assumed: 3 kW at 300 V)

%% DC link 1 (not measured)
Vdc1     = 400;         % V, nominal
Vdc1Rip  = 7;           % V, amplitude of the ripple (Beni: about 7 V)
fRip     = 50;          % Hz, ripple frequency

%% Control
fCtrl    = 9.6667e3;    % Hz, control frequency
Ts       = 1/fCtrl;     % s
VbuckSet = 300;         % V, DC link 2 setpoint (output of PI controller 1, constant here)
mMax     = 0.95;        % modulation limit
mMin     = 0;

% PI controller 2 (integral only: a P part excites the lightly damped LC
% resonance at 1.7 kHz; this Ki keeps the loop stable down to no load)
Kp = 0;                 % 1/V
Ki = 0.3;               % 1/(V s)

% Resonant term at fRip, in parallel with the PI:
%   R(s) = Kr * s / (s^2 + 2*wc*s + w0^2),   gain Kr/(2*wc) at w0
% Discretised with Tustin, prewarped at w0, so the peak stays exactly at fRip.
Kr = 0.5;               % 1/V
wc = 2*pi*0.5;          % rad/s, half bandwidth of the resonance (+-0.5 Hz)
w0 = 2*pi*fRip;
resEnable = 1;          % 1: PI + resonant, 0: PI only

% Discrete resonant term (computed with base MATLAB, no toolbox):
% Tustin with prewarping, s = k*(z-1)/(z+1), k = w0/tan(w0*Ts/2).
k   = w0/tan(w0*Ts/2);
a0  = k^2 + 2*wc*k + w0^2;
resNum = Kr*k*[1, 0, -1]/a0;
resDen = [1, (2*w0^2 - 2*k^2)/a0, (k^2 - 2*wc*k + w0^2)/a0];
clear k a0

%% Operating point (start in steady state)
iL0 = VbuckSet/Rload;
vC0 = VbuckSet;
m0  = (VbuckSet*(1 + RL/Rload))/Vdc1;
