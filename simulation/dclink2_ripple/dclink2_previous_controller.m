function c = dclink2_previous_controller(Ts, L, C, RL, Rnom, Vdc1)
%DCLINK2_PREVIOUS_CONTROLLER Controller of 2026-11-17 (before RL = 100 mOhm).
%
%   c = dclink2_previous_controller(Ts, L, C, RL, Rnom, Vdc1) returns the
%   previous design for comparison: integral only (Ki = 0.3), resonant terms
%   at 50..300 Hz and a notch at the LC resonance (1.6 kHz, zeta 0.15/0.7)
%   on the error. Fields: name, Kp, Ki, filtNum, filtDen, resA..resD, Kr.

    wn = 2*pi*1600;
    K  = wn/tan(wn*Ts/2);
    p  = @(zeta) [K^2 + 2*zeta*wn*K + wn^2, 2*wn^2 - 2*K^2, K^2 - 2*zeta*wn*K + wn^2];
    den = p(0.7);
    c.name    = 'previous (Ki 0.3, notch)';
    c.Kp      = 0;
    c.Ki      = 0.3;
    c.filtNum = p(0.15)/den(1);
    c.filtDen = den/den(1);
    c.Kr      = [0.5 0.25 0.2 0.15 0.12 0.1];
    [c.resA, c.resB, c.resC, c.resD] = dclink2_resonant_coeffs((1:6)*50, c.Kr, 2*pi*0.5, Ts, ...
        L, C, RL, Rnom, Vdc1, c.filtNum, c.filtDen);
end
