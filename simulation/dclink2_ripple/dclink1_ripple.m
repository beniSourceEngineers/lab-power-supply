function [amp, phase, h] = dclink1_ripple(P, Pnom)
%DCLINK1_RIPPLE Harmonic ripple of DC link 1 at an operating point.
%
%   [amp, phase, h] = dclink1_ripple(P, Pnom) returns the ripple of DC link 1
%   as a sum of harmonics of the grid frequency fg:
%
%       vdc1(t) = Vdc1 + sum_k amp(k) * sin(2*pi*h(k)*fg*t + phase(k))
%
%   for the load power P (W), relative to the nominal power Pnom (W).
%
%   Source: single-phase PFC that is not well synchronised to the grid. The
%   ripple is not a pure 50 Hz sine; it contains harmonics of 50 Hz whose
%   amplitude and phase depend on the operating point, with 50 Hz dominant
%   (about 7 V at nominal power, Beni 2026-11-16/17).
%
%   ASSUMED spectrum (replace by a measured spectrum of DC link 1 at a few
%   operating points as soon as one is available):
%     - amplitude at nominal power: 7 V at 50 Hz, falling with the harmonic
%       order (ampNom below), up to the 10th harmonic,
%     - amplitudes scale with the load: 30 % at no load to 100 % at Pnom
%       (the 100 Hz component, the natural single-phase power ripple, scales
%       fully with P),
%     - phases shift with the load (fixed random base phase + h * 40 deg * P/Pnom).

    h       = 1:10;
    ampNom  = [7.0 3.0 2.0 1.2 1.0 0.7 0.5 0.4 0.3 0.25];   % V at Pnom
    scale   = 0.3 + 0.7*min(max(P/Pnom, 0), 1.5);
    amp     = ampNom * scale;
    amp(2)  = ampNom(2) * min(max(P/Pnom, 0), 1.5);

    basePhase = [0.3 2.1 -1.2 0.8 2.9 -2.4 1.5 -0.4 2.2 -1.8];   % rad, fixed "random"
    phase   = basePhase + h * deg2rad(40) * (P/Pnom);
end
