function report = design_dclink2_ripple()
%DESIGN_DCLINK2_RIPPLE Linear design check of PI controller 2 + resonant terms.
%
%   Checks controllers on the discrete plant m -> vdc2 (ZOH, one sample
%   computation delay) over a grid of operating points and tolerances:
%     load      Rnom .. 100*Rnom (3 kW .. 30 W at 300 V)
%     DC link 1 Vdc1 -+ 2 %
%     L, C      -+ 10 %
%   Designs:
%     new        dclink2_ripple_params.m (Kp, Ki, low-pass, resonant terms)
%     previous   dclink2_previous_controller.m (Ki 0.3, notch, resonant terms)
%     no filter  the new controller without the low-pass: shows that
%                RL = 100 mOhm alone does not allow this P part
%   Reported per design: stability, worst sensitivity peak Ms, worst |S| at
%   10 Hz (rejection of slow disturbances, e.g. load changes) and at each
%   ripple harmonic (1..10); the new design also with coefficients rounded
%   to single precision (C28x FPU32). Bode plot of S:
%   <tempdir>/design_dclink2_ripple.png. Needs the Control System Toolbox.

    here = fileparts(mfilename('fullpath'));
    oldPath = addpath(here);
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>
    run(fullfile(here, 'dclink2_ripple_params.m'));

    newC = struct('name', 'new (Kp 1e-3, Ki 2, low-pass)', 'Kp', Kp, 'Ki', Ki, ... %#ok<NODEF>
        'filtNum', filtNum, 'filtDen', filtDen, 'resA', resA, 'resB', resB, 'resC', resC, 'resD', resD);
    prev = dclink2_previous_controller(Ts, L, C, RL, Rnom, Vdc1);
    prev = rmfield(prev, 'Kr');
    noFilt = newC; noFilt.name = 'new without low-pass'; noFilt.filtNum = 1; noFilt.filtDen = 1;
    single_ = newC; single_.name = 'new, single-precision coefficients';
    for f = {'filtNum', 'filtDen', 'resA', 'resB', 'resC', 'resD'}
        single_.(f{1}) = double(single(single_.(f{1})));
    end
    designs = [newC, single_, prev, noFilt];

    loads = Rnom*[1 3 10 100];
    vdcs  = Vdc1*[0.98 1 1.02];
    tolL  = [0.9 1.1];
    tolC  = [0.9 1.1];
    hAll  = 1:10;
    w = 2*pi*logspace(0, log10(0.999/(2*Ts)), 20000);
    z = tf('z', Ts);

    report = struct();
    figure('Visible', 'off', 'Position', [100 100 1000 450]); ax = gca; hold(ax, 'on'); grid(ax, 'on');
    for d = 1:numel(designs)
        c = designs(d);
        Ctot = (ss(c.Kp + c.Ki*Ts*z/(z - 1)) + ss(c.resA, c.resB, c.resC, c.resD, Ts)) ...
            * ss(tf(c.filtNum, c.filtDen, Ts, 'Variable', 'z^-1'));

        stable = true; Ms = -inf; S10 = -inf; Sh = -inf(size(hAll)); worst = ''; Sworst = [];
        for R = loads, for V = vdcs, for kL = tolL, for kC = tolC
            P  = plantd(L*kL, C*kC, RL, R, V, Ts);
            Sc = feedback(ss(1, 'Ts', Ts), Ctot*P);
            if ~all(abs(eig(Sc)) < 1)
                stable = false;
                worst = sprintf('unstable at R=%g ohm, Vdc1=%g V, L*%g, C*%g', R, V, kL, kC);
                continue
            end
            mag = abs(squeeze(freqresp(Sc, w)));
            if stable && 20*log10(max(mag)) > Ms
                Ms = 20*log10(max(mag));
                worst = sprintf('R=%g ohm, Vdc1=%g V, L*%g, C*%g', R, V, kL, kC);
                Sworst = Sc;
            end
            S10 = max(S10, 20*log10(abs(freqresp(Sc, 2*pi*10))));
            Sh = max(Sh, 20*log10(abs(squeeze(freqresp(Sc, 2*pi*fRes*hAll)))).');
        end, end, end, end

        report.(matlab.lang.makeValidName(c.name)) = struct('stable', stable, 'MsdB', Ms, ...
            'S10dB', S10, 'SdB', Sh, 'worstCase', worst);
        if ~stable
            fprintf('\n%s: UNSTABLE (%s)\n', c.name, worst);
            continue
        end
        fprintf('\n%s: worst Ms = %.1f dB (%s), worst |S(10 Hz)| = %.1f dB\n', c.name, Ms, worst, S10);
        fprintf('  worst |S| at harmonic   '); fprintf('%6d', hAll); fprintf('\n');
        fprintf('  [dB]                    '); fprintf('%6.1f', Sh); fprintf('\n');
        if ~contains(c.name, 'single')
            mag = abs(squeeze(freqresp(Sworst, w)));
            semilogx(ax, w/2/pi, 20*log10(mag), 'DisplayName', [c.name ' (worst case)']);
        end
    end
    set(ax, 'XScale', 'log');
    xlabel(ax, 'f [Hz]'); ylabel(ax, '|S| [dB]');
    title(ax, sprintf('Sensitivity of the DC link 2 loop, worst case over load, Vdc1, L, C (RL = %g mOhm)', 1e3*RL));
    xline(ax, fRes*hAll, ':', 'HandleVisibility', 'off'); xlim(ax, [1 5000]);
    legend(ax, 'show', 'Location', 'southeast');
    file = fullfile(tempdir, 'design_dclink2_ripple.png');
    exportgraphics(gcf, file, 'Resolution', 150); close(gcf);
    fprintf('\nPlot: %s\n', file);

    % Firmware coefficients (single precision, direct form II transposed)
    fprintf('\nKp = %gF, Ki*Ts = %.9eF\n', Kp, Ki*Ts);
    fprintf('Low-pass (on the error): b = [%.9eF, %.9eF, %.9eF], a1 = %.9fF, a2 = %.9fF\n', ...
        filtNum, filtDen(2), filtDen(3));
    fprintf('Resonant biquads (y = b0*e + s1; s1 = b1*e - a1*y + s2; s2 = b2*e - a2*y):\n');
    for k = 1:numel(resBiquads)
        bq = resBiquads(k);
        fprintf('  %4g Hz: b = [%.9eF, %.9eF, %.9eF], a1 = %.9fF, a2 = %.9fF, theta = %.1f deg\n', ...
            bq.f, bq.b, bq.a(2), bq.a(3), rad2deg(bq.theta));
    end
end

function P = plantd(L, C, RL, R, Vdc1, Ts)
    % m -> vdc2, ZOH, plus one sample computation delay.
    s = tf('s');
    G = ss(Vdc1/(L*C*s^2 + (L/R + RL*C)*s + 1 + RL/R));
    P = c2d(G, Ts, 'zoh');
    P.InputDelay = 1;
    P = absorbDelay(P);
end
