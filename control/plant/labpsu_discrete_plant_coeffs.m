function p = labpsu_discrete_plant_coeffs(L, C, R, RL, Vin, Ts)
%LABPSU_DISCRETE_PLANT_COEFFS Discrete low-pass filters of the buck plant.
%
%   p = labpsu_discrete_plant_coeffs(L, C, R, RL, Vin, Ts) discretizes the
%   averaged buck model of LabPsu_plant.slx
%
%       L  diL/dt = d*Vin - vC - RL*iL
%       C  dvC/dt = iL - vC/R
%
%   with zero-order hold at sample time Ts and returns the transfer
%   functions duty -> vo and duty -> iL as Discrete Transfer Fcn
%   coefficients (ascending powers of z^-1):
%
%       p.numV, p.numI   numerators   (3 elements, first one is 0)
%       p.den            common denominator (3 elements, p.den(1) = 1)
%       p.Ts             sample time
%
%   Continuous form, for reference:
%       vo/d = Vin*R        / (L*C*R*s^2 + (L + RL*R*C)*s + R + RL)
%       iL/d = Vin*(R*C*s+1)/ (L*C*R*s^2 + (L + RL*R*C)*s + R + RL)
%
%   ZOH is exact at the sample instants when d is constant between samples,
%   which holds when d comes from the control loop running at Ts.
%   Only base MATLAB is used (expm), no Control System Toolbox.
%
%   Used by LabPsu_discretePlant.slx (InitFcn).

    L = double(L); C = double(C); R = double(R); RL = double(RL);
    Vin = double(Vin); Ts = double(Ts);

    % x = [iL; vC], u = d
    A = [-RL/L, -1/L;
          1/C,  -1/(R*C)];
    B = [Vin/L; 0];

    Ad = expm(A*Ts);
    Bd = A \ ((Ad - eye(2))*B);

    % (zI - Ad)^-1 = adj(zI - Ad)/det(zI - Ad), adj(zI - Ad) = z*I + M
    M = [-Ad(2,2),  Ad(1,2);
          Ad(2,1), -Ad(1,1)];

    p.Ts   = Ts;
    p.den  = [1, -trace(Ad), det(Ad)];
    p.numV = coeffs([0 1], Bd, M);
    p.numI = coeffs([1 0], Bd, M);

    % Continuous parameters, for information only.
    p.wn    = sqrt((R + RL)/(L*C*R));
    p.zeta  = (L + RL*R*C)/(2*L*C*R*p.wn);
    p.dcV   = Vin*R/(R + RL);
    p.dcI   = Vin/(R + RL);
end

function num = coeffs(c, Bd, M)
    num = [0, c*Bd, c*M*Bd];
end
