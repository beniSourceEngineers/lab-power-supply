%BUCK_PLANT_PARAMS Parameters of buck_plant.slx (model workspace data source).
%
%   Averaged buck from DC link 1 (Vin) to DC link 2 (Vout), CCM:
%       L  diL/dt = m*Vin - Rseries*iL - vC
%       C  dvC/dt = iL - iLoad,   iLoad = vC * Pout/Vout^2   (Rload = Vout^2/Pout)
%   plus the analog RC filter of the Vout measurement.
%   Not valid in DCM (light load): check Lcrit = Rload*(1 - m)/(2*fsw) < L.

L       = 220e-6;       % H
C       = 40e-6;        % F
Rseries = 2;            % ohm, series resistance (inductor, switches, wiring)

Rfilt_1 = 10e3;         % ohm, analog filter of the Vout measurement
Cfilt_1 = 10e-9;        % F

% Sample time of the discrete plant (control rate 58 kHz/2/3 = 9.667 kHz)
TsPlant = 1/(58e3/2/3);

% Initial state [iL; vC] (0: start discharged)
iL0 = 0;
vC0 = 0;

%% Discrete state-space model (exact ZOH at TsPlant)
%   x = [iL; vC; vMeas],  u = [m*Vin; iLoad],  y = x
%   vMeas: vC after the analog RC filter.
% The load is an input (iLoad = vC*Pout/Vout^2, computed from x(k)), so the
% matrices do not depend on the operating point and Vin, Vout, Pout can
% change at runtime. Computed with base MATLAB (expm).
tauFilt = Rfilt_1*Cfilt_1;
Ac = [-Rseries/L, -1/L,       0;
       1/C,        0,         0;
       0,          1/tauFilt, -1/tauFilt];
Bc = [1/L,  0;
      0,   -1/C;
      0,    0];
Mz = expm([Ac, Bc; zeros(2, 5)]*TsPlant);
Ad = Mz(1:3, 1:3);
Bd = Mz(1:3, 4:5);
Cd = eye(3);
Dd = zeros(3, 2);
clear Mz
