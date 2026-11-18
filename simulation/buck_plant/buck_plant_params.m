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

% Initial state [iL; vC] (0: start discharged)
iL0 = 0;
vC0 = 0;
