function report = design_dclink2_ripple()
%DESIGN_DCLINK2_RIPPLE Linear design check of PI controller 2 + resonant terms.
%
%   Checks the controller in dclink2_ripple_params.m on the discrete plant
%   m -> vdc2 (ZOH, one sample computation delay) over a grid of operating
%   points and tolerances:
%     load      Rnom .. 100*Rnom (3 kW .. 30 W at 300 V)
%     DC link 1 Vdc1 -+ 2 %
%     L, C      -+ 10 %
%   and compares it with the single 50 Hz resonant term (previous design).
%   Reported per design: stability, worst sensitivity peak Ms and the
%   worst-case attenuation |S| at each ripple harmonic (1..10), with the
%   coefficients in double and rounded to single precision (C28x FPU32).
%   A Bode plot of S is saved to <tempdir>/design_dclink2_ripple.png.
%   Needs the Control System Toolbox.

    here = fileparts(mfilename('fullpath'));
    oldPath = addpath(here);
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>
    run(fullfile(here, 'dclink2_ripple_params.m'));

    z = tf('z', Ts); %#ok<NODEF>
    Cpi = ss(Kp + Ki*Ts*z/(z - 1));
    Notch = ss(tf(notchNum, notchDen, Ts, 'Variable', 'z^-1'));
    hAll = 1:10;

    designs = struct('name', {'previous: 50 Hz only', 'new: harmonics 1..6 + LC notch'}, ...
        'h', {1, resH}, 'Kr', {0.5, resKr}, 'notch', {false, true});

    loads = Rnom*[1 3 10 100];
    vdcs  = Vdc1*[0.98 1 1.02];
    tolL  = [0.9 1.1];
    tolC  = [0.9 1.1];
    w = 2*pi*logspace(0, log10(0.999/(2*Ts)), 20000);

    report = struct();
    figure('Visible', 'off', 'Position', [100 100 1000 450]); ax = gca; hold(ax, 'on'); grid(ax, 'on');
    for d = 1:numel(designs)
        for prec = {'double', 'single'}
            if designs(d).notch
                [rA, rB, rC, rD] = dclink2_resonant_coeffs(designs(d).h*fRes, designs(d).Kr, wc, Ts, ...
                    L, C, RL, Rnom, Vdc1, notchF, notchZetaZ, notchZetaP);
            else
                [rA, rB, rC, rD] = dclink2_resonant_coeffs(designs(d).h*fRes, designs(d).Kr, wc, Ts, ...
                    L, C, RL, Rnom, Vdc1);
            end
            if strcmp(prec{1}, 'single')
                rA = double(single(rA)); rB = double(single(rB)); rC = double(single(rC)); rD = double(single(rD));
            end
            Cres = ss(rA, rB, rC, rD, Ts);
            Ctot = Cpi + Cres;
            if designs(d).notch
                N = Notch;
                if strcmp(prec{1}, 'single')
                    N = ss(tf(double(single(notchNum)), double(single(notchDen)), Ts, 'Variable', 'z^-1'));
                end
                Ctot = Ctot*N;
            end

            stable = true; Ms = -inf; Sh = -inf(size(hAll)); worst = '';
            for R = loads, for V = vdcs, for kL = tolL, for kC = tolC
                P  = plantd(L*kL, C*kC, RL, R, V, Ts);
                Sc = feedback(ss(1, 'Ts', Ts), Ctot*P);
                stable = stable && all(abs(eig(Sc)) < 1);
                mag = abs(squeeze(freqresp(Sc, w)));
                if 20*log10(max(mag)) > Ms
                    Ms = 20*log10(max(mag));
                    worst = sprintf('R=%g ohm, Vdc1=%g V, L*%g, C*%g', R, V, kL, kC);
                    Sworst = Sc;
                end
                Sh = max(Sh, 20*log10(abs(squeeze(freqresp(Sc, 2*pi*fRes*hAll)))).');
            end, end, end, end

            key = matlab.lang.makeValidName([designs(d).name '_' prec{1}]);
            report.(key) = struct('stable', stable, 'MsdB', Ms, 'worstCase', worst, 'SdB', Sh);
            fprintf('\n%s (%s coefficients): stable=%d, worst Ms=%.1f dB at %s\n', ...
                designs(d).name, prec{1}, stable, Ms, worst);
            fprintf('  worst |S| at harmonic   '); fprintf('%6d', hAll); fprintf('\n');
            fprintf('  [dB]                    '); fprintf('%6.1f', Sh); fprintf('\n');
            if strcmp(prec{1}, 'double')
                mag = abs(squeeze(freqresp(Sworst, w)));
                semilogx(ax, w/2/pi, 20*log10(mag), 'DisplayName', [designs(d).name ' (worst case)']);
            end
        end
    end
    xlabel(ax, 'f [Hz]'); ylabel(ax, '|S| = |vdc2 ripple closed / open loop| [dB]');
    title(ax, 'Sensitivity of the DC link 2 loop, worst case over load, Vdc1, L, C');
    xline(ax, fRes*hAll, ':', 'HandleVisibility', 'off'); xlim(ax, [10 5000]);
    legend(ax, 'show', 'Location', 'southeast');
    file = fullfile(tempdir, 'design_dclink2_ripple.png');
    exportgraphics(gcf, file, 'Resolution', 150); close(gcf);
    fprintf('\nPlot: %s\n', file);

    % Firmware coefficients (single precision, direct form II transposed)
    [~, ~, ~, ~, bq] = dclink2_resonant_coeffs(resH*fRes, resKr, wc, Ts, L, C, RL, Rnom, Vdc1, ...
        notchF, notchZetaZ, notchZetaP);
    fprintf('\nLC notch (on the error): b = [%.9fF, %.9fF, %.9fF], a1 = %.9fF, a2 = %.9fF\n', ...
        notchNum, notchDen(2), notchDen(3));
    fprintf('\nResonant biquads (y = b0*e + s1; s1 = b1*e - a1*y + s2; s2 = b2*e - a2*y):\n');
    for k = 1:numel(bq)
        fprintf('  %4g Hz: b = [%.9eF, %.9eF, %.9eF], a1 = %.9fF, a2 = %.9fF, theta = %.1f deg\n', ...
            bq(k).f, bq(k).b, bq(k).a(2), bq(k).a(3), rad2deg(bq(k).theta));
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
