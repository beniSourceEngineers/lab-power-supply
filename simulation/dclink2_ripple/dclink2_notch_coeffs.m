function [num, den] = dclink2_notch_coeffs(fn, zetaZ, zetaP, Ts)
%DCLINK2_NOTCH_COEFFS Discrete notch filter of PI controller 2.
%
%   [num, den] = dclink2_notch_coeffs(fn, zetaZ, zetaP, Ts) discretises
%
%       N(s) = (s^2 + 2*zetaZ*wn*s + wn^2) / (s^2 + 2*zetaP*wn*s + wn^2),  wn = 2*pi*fn
%
%   with Tustin prewarped at wn. num, den: coefficients in z^-1, den(1) = 1.
%   DC gain 1. Base MATLAB only.

    wn = 2*pi*fn;
    K  = wn/tan(wn*Ts/2);
    p  = @(zeta) [K^2 + 2*zeta*wn*K + wn^2, 2*wn^2 - 2*K^2, K^2 - 2*zeta*wn*K + wn^2];
    num = p(zetaZ);
    den = p(zetaP);
    num = num/den(1);
    den = den/den(1);
end
