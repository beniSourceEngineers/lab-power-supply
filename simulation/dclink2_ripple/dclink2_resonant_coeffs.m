function [A, B, Cm, D, bq] = dclink2_resonant_coeffs(f, Kr, wc, Ts, L, C, RL, R, Vdc1, fn, zetaZ, zetaP)
%DCLINK2_RESONANT_COEFFS Discrete resonant terms for PI controller 2.
%
%   [A, B, Cm, D, bq] = dclink2_resonant_coeffs(f, Kr, wc, Ts, L, C, RL, R, Vdc1, fn, zetaZ, zetaP)
%   discretises, for every frequency f(k) (Hz),
%
%       R_k(s) = Kr(k) * (s*cos(th_k) - w_k*sin(th_k)) / (s^2 + 2*wc*s + w_k^2)
%
%   with Tustin prewarped at w_k = 2*pi*f(k). th_k = -angle(Gd(j*w_k)) is the
%   phase of the plant m -> vdc2 (buck LC with load R, input Vdc1) including
%   the loop delay of 1.5*Ts (1 sample computation + ZOH) and the notch at fn
%   (zetaZ, zetaP; omit them for no notch), so that R_k*N*Gd is real and
%   positive at w_k.
%
%   bq(k) holds the biquad coefficients of R_k as the firmware runs them
%   (direct form II transposed):
%       y    = b0*e + s1
%       s1   = b1*e - a1*y + s2
%       s2   = b2*e - a2*y
%   A, B, Cm, D: the sum of all biquads as one discrete state-space system
%   (2 states per term), used by the Simulink model.
%   Base MATLAB only.

    n = numel(f);
    A = zeros(2*n); B = zeros(2*n, 1); Cm = zeros(1, 2*n); D = 0;
    bq = struct('f', {}, 'theta', {}, 'b', {}, 'a', {});
    for k = 1:n
        w  = 2*pi*f(k);
        jw = 1j*w;
        G  = Vdc1/(L*C*jw^2 + (L/R + RL*C)*jw + 1 + RL/R) * exp(-jw*1.5*Ts);
        if nargin >= 12
            wn = 2*pi*fn;
            G  = G * (jw^2 + 2*zetaZ*wn*jw + wn^2)/(jw^2 + 2*zetaP*wn*jw + wn^2);
        end
        th = -angle(G);

        K  = w/tan(w*Ts/2);
        a0 = K^2 + 2*wc*K + w^2;
        b  = Kr(k)*[K*cos(th) - w*sin(th), -2*w*sin(th), -K*cos(th) - w*sin(th)]/a0;
        a  = [1, (2*w^2 - 2*K^2)/a0, (K^2 - 2*wc*K + w^2)/a0];

        idx = 2*k-1:2*k;
        A(idx, idx) = [-a(2) 1; -a(3) 0];
        B(idx)      = [b(2) - a(2)*b(1); b(3) - a(3)*b(1)];
        Cm(idx)     = [1 0];
        D           = D + b(1);
        bq(k) = struct('f', f(k), 'theta', th, 'b', b, 'a', a); %#ok<AGROW>
    end
end
