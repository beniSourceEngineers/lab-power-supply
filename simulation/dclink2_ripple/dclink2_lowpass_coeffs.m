function [num, den] = dclink2_lowpass_coeffs(fc, zeta, Ts)
%DCLINK2_LOWPASS_COEFFS Second-order low-pass on the error of PI controller 2.
%
%   [num, den] = dclink2_lowpass_coeffs(fc, zeta, Ts) discretises
%
%       F(s) = wf^2 / (s^2 + 2*zeta*wf*s + wf^2),  wf = 2*pi*fc
%
%   with Tustin prewarped at wf. num, den: coefficients in z^-1, den(1) = 1.
%   DC gain 1. Base MATLAB only.

    wf = 2*pi*fc;
    K  = wf/tan(wf*Ts/2);
    a  = [K^2 + 2*zeta*wf*K + wf^2, 2*wf^2 - 2*K^2, K^2 - 2*zeta*wf*K + wf^2];
    num = wf^2*[1 2 1]/a(1);
    den = a/a(1);
end
