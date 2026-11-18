%RESONANT_EXAMPLE PI controller 2 with resonant terms, no filter in front.
%
%   Self-contained example (does not use the model or the other scripts):
%     1. buck plant m -> vdc2 (L, C, RL, load R, DC link 1 voltage), ZOH and
%        one sample computation delay,
%     2. integral part (Kp = 0, Ki = 0.3) plus resonant terms at 50..300 Hz,
%        each with phase compensation for plant + delay, discretised with
%        Tustin prewarped at its frequency. No low-pass or notch on the error.
%     3. stability and sensitivity peak over load and L, C tolerances,
%     4. Bode plots of the open loop and the sensitivity,
%     5. the controller run sample by sample as the firmware would (single
%        precision, DF2T biquads) on the plant, with the DC link 1 ripple
%        measured from the scope photo of 2026-11-18.
%   Steps 3 and 4 need the Control System Toolbox; 1, 2 and 5 base MATLAB only.

%% Parameters
L    = 220e-6;          % H
C    = 40e-6;           % F
RL   = 0.1;             % ohm
Rnom = 30;              % ohm (3 kW at 300 V)
Vdc1 = 400;             % V
Vset = 300;             % V, DC link 2 setpoint
Ts   = 1/9.6667e3;      % s
Ki   = 0.3;             % 1/(V s), integral (Kp = 0)

fRes = 50*(1:6);                          % Hz, resonant frequencies
Kr   = [0.5 0.25 0.2 0.15 0.12 0.1];      % 1/V, gain Kr/(2*wc) at each frequency
wc   = 2*pi*0.5;                          % rad/s, half bandwidth (+-0.5 Hz)

% DC link 1 ripple (scope photo 2026-11-18, harmonics of 50 Hz)
ampNom    = [1.77 0.71 0.09 0.06 0.06 0.03 0.01 0.01 0.01 0.02];     % V
basePhase = [-1.49 1.25 2.22 -1.65 -1.52 1.78 0.11 0.98 -1.66 0.33]; % rad

%% Resonant terms (no filter): R_k(s) = Kr*(s*cos(th) - w*sin(th))/(s^2 + 2*wc*s + w^2)
% th = -angle of plant + delay at w, so that R_k*P is real and positive at w.
% Biquad in z^-1, run as DF2T: y = b0*e + s1; s1 = b1*e - a1*y + s2; s2 = b2*e - a2*y
% resA, resB, resC, resD: all biquads summed as one discrete state-space
% system (states [s1; s2] per term), e.g. for a Discrete State-Space block.
% bq(k): coefficients per term.
nRes = numel(fRes);
b = zeros(nRes, 3); a = zeros(nRes, 3); theta = zeros(1, nRes);
resA = zeros(2*nRes); resB = zeros(2*nRes, 1); resC = zeros(1, 2*nRes); resD = 0;
bq = struct('f', {}, 'theta', {}, 'b', {}, 'a', {});
for k = 1:nRes
    w  = 2*pi*fRes(k); jw = 1j*w;
    P  = Vdc1/(L*C*jw^2 + (L/Rnom + RL*C)*jw + 1 + RL/Rnom) * exp(-jw*1.5*Ts);
    th = -angle(P);
    K  = w/tan(w*Ts/2);                   % Tustin prewarped at w
    a0 = K^2 + 2*wc*K + w^2;
    b(k, :) = Kr(k)*[K*cos(th) - w*sin(th), -2*w*sin(th), -K*cos(th) - w*sin(th)]/a0;
    a(k, :) = [1, (2*w^2 - 2*K^2)/a0, (K^2 - 2*wc*K + w^2)/a0];
    theta(k) = th;

    idx = 2*k-1:2*k;
    resA(idx, idx) = [-a(k, 2) 1; -a(k, 3) 0];
    resB(idx)      = [b(k, 2) - a(k, 2)*b(k, 1); b(k, 3) - a(k, 3)*b(k, 1)];
    resC(idx)      = [1 0];
    resD           = resD + b(k, 1);
    bq(k) = struct('f', fRes(k), 'theta', th, 'b', b(k, :), 'a', a(k, :));
end

fprintf('Ki*Ts = %.9eF\n', Ki*Ts);
fprintf('Resonant biquads (DF2T):\n');
for k = 1:nRes
    fprintf('  %3d Hz: b = [%.9eF, %.9eF, %.9eF], a1 = %.9fF, a2 = %.9fF, theta = %5.1f deg\n', ...
        fRes(k), b(k, :), a(k, 2), a(k, 3), rad2deg(theta(k)));
end

%% Stability and sensitivity over load and tolerances (Control System Toolbox)
z = tf('z', Ts); s = tf('s');
Cz = ss(Ki*Ts*z/(z - 1)) + ss(resA, resB, resC, resD, Ts);
plantOf = @(R, kL, kC) absorbDelay(setfield(c2d(ss(Vdc1/(L*kL*C*kC*s^2 + (L*kL/R + RL*C*kC)*s + 1 + RL/R)), ...
    Ts, 'zoh'), 'InputDelay', 1)); %#ok<SFLD>
fgrid = logspace(0, log10(0.999/(2*Ts)), 5000);
fprintf('\n%-8s %-6s %-6s %8s %10s %10s\n', 'load', 'L', 'C', 'stable', 'Ms [dB]', 'at [Hz]');
worst = -inf;
for R = Rnom*[1 10 100]
    for kL = [0.9 1 1.1]
        for kC = [0.9 1 1.1]
            Sc = feedback(ss(1, 'Ts', Ts), Cz*plantOf(R, kL, kC));
            st = all(abs(eig(Sc)) < 1);
            mag = abs(squeeze(freqresp(Sc, 2*pi*fgrid)));
            [ms, i] = max(mag);
            worst = max(worst, 20*log10(ms));
            if kL ~= 1 || kC ~= 1, continue; end
            fprintf('%-8s %-6g %-6g %8d %10.1f %10.0f\n', sprintf('%.0f W', Vset^2/R), kL, kC, st, ...
                20*log10(ms), fgrid(i));
        end
    end
end
fprintf('worst Ms over load and L, C +-10 %%: %.1f dB\n', worst);

%% Bode plots: open loop and sensitivity at three loads
figure('Name', 'Resonant example', 'Position', [100 100 1000 700]);
tl = tiledlayout(3, 1, 'TileSpacing', 'compact');
title(tl, 'PI controller 2 (Ki 0.3) + resonant terms 50..300 Hz, no filter, RL = 100 m\Omega');
axM = nexttile; axP = nexttile; axS = nexttile;
hold([axM axP axS], 'on'); grid([axM axP axS], 'on'); set([axM axP axS], 'XScale', 'log');
for R = Rnom*[1 10 100]
    Pk = plantOf(R, 1, 1);
    h  = squeeze(freqresp(Cz*Pk, 2*pi*fgrid));
    hs = squeeze(freqresp(feedback(ss(1, 'Ts', Ts), Cz*Pk), 2*pi*fgrid));
    lbl = sprintf('%.0f W', Vset^2/R);
    semilogx(axM, fgrid, 20*log10(abs(h)), 'DisplayName', lbl);
    semilogx(axP, fgrid, rad2deg(angle(h)), 'DisplayName', lbl);
    semilogx(axS, fgrid, 20*log10(abs(hs)), 'DisplayName', lbl);
end
ylabel(axM, '|L| [dB]'); ylabel(axP, '\angle L [deg]'); ylabel(axS, '|S| [dB]'); xlabel(axS, 'f [Hz]');
yline(axM, 0, 'k:'); yline(axS, 6, 'r:', 'Ms = 6 dB');
legend(axM, 'show', 'Location', 'southwest');
linkaxes([axM axP axS], 'x'); xlim(axM, [fgrid(1) fgrid(end)]);

%% Firmware-style run (single precision) on the averaged plant
% Plant integrated with exact ZOH steps between samples; DC link 1 ripple from
% the photo; one sample computation delay; nominal load.
Tend = 1.0; N = round(Tend/Ts); t = (0:N-1)*Ts;
A  = [-RL/L, -1/L; 1/C, -1/(Rnom*C)];
Ad = expm(A*Ts); Bd = A\((Ad - eye(2))*[1/L; 0]);
x  = [Vset/Rnom; Vset];
m0 = (Vset + RL*Vset/Rnom)/Vdc1;
I  = single(m0); st = zeros(nRes, 2, 'single');
bS = single(b); aS = single(a);
mDelayed = single(m0);
vdc2 = zeros(1, N); mLog = zeros(1, N);
resOn = t >= 0.5;                                   % resonant terms on after 0.5 s
for n = 1:N
    vdc1 = Vdc1 + sum(ampNom .* sin(2*pi*50*(1:10)*t(n) + basePhase));
    % --- controller (as in the firmware) ---
    e = single(Vset - x(2));
    I = min(max(I + single(Ki*Ts)*e, single(0)), single(0.95));
    r = single(0);
    if resOn(n)
        for k = 1:nRes
            y = bS(k, 1)*e + st(k, 1);
            st(k, 1) = bS(k, 2)*e - aS(k, 2)*y + st(k, 2);
            st(k, 2) = bS(k, 3)*e - aS(k, 3)*y;
            r = r + y;
        end
    end
    m = min(max(I + r, single(0)), single(0.95));
    % --- plant: m from the previous step is applied now (computation delay) ---
    x = Ad*x + Bd*(double(mDelayed)*vdc1);
    mDelayed = m;
    vdc2(n) = x(2); mLog(n) = double(m);
end
pp = @(sel) max(vdc2(sel)) - min(vdc2(sel));
fprintf('\nDC link 2 ripple, 3 kW: PI only %.2f V pp, PI + resonant %.2f V pp\n', ...
    pp(t > 0.3 & t < 0.5), pp(t > 0.8));

figure('Name', 'Resonant example, time domain', 'Position', [150 150 1000 400]);
plot(t, vdc2); grid on; xline(0.5, 'r--', 'resonant terms on');
xlabel('t [s]'); ylabel('v_{dc2} [V]');
title('DC link 2 with the measured DC link 1 ripple, firmware-style controller');
