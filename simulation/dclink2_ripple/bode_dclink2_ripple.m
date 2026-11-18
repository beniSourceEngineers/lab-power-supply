function files = bode_dclink2_ripple()
%BODE_DCLINK2_RIPPLE Bode plots of the DC link 2 loop.
%
%   files = bode_dclink2_ripple() plots, for the new controller
%   (dclink2_ripple_params.m) and the previous one
%   (dclink2_previous_controller.m), over the load range 30 W .. 3 kW:
%     1  plant        m -> vdc2 (LC with RL, ZOH, one sample delay)
%     2  controller   e -> m (low-pass/notch, P, I, resonant terms)
%     3  open loop    L = C*P, with gain and phase margins
%     4  sensitivity  S = 1/(1+L) and T = L/(1+L)
%     5  ripple       vdc1 -> vdc2, open loop (no control) and closed loop
%   The ripple harmonics (50 Hz multiples) are marked. PNG files are saved to
%   <tempdir>/bode_dclink2_*.png. Needs the Control System Toolbox.

    here = fileparts(mfilename('fullpath'));
    oldPath = addpath(here);
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>
    run(fullfile(here, 'dclink2_ripple_params.m'));

    z = tf('z', Ts); %#ok<NODEF>
    s = tf('s');
    prev = dclink2_previous_controller(Ts, L, C, RL, Rnom, Vdc1);
    ctrls = { ...
        'new (Kp 1e-3, Ki 2, low-pass)', ctrlOf(Kp, Ki, filtNum, filtDen, resA, resB, resC, resD, Ts), ...
            ctrlOf(Kp, Ki, filtNum, filtDen, resA, resB, 0*resC, 0, Ts); ...
        'previous (Ki 0.3, notch)',      ctrlOf(prev.Kp, prev.Ki, prev.filtNum, prev.filtDen, ...
                                                prev.resA, prev.resB, prev.resC, prev.resD, Ts), ...
            ctrlOf(prev.Kp, prev.Ki, prev.filtNum, prev.filtDen, prev.resA, prev.resB, 0*prev.resC, 0, Ts)};
    loads  = Rnom*[1 10 100];
    lnames = arrayfun(@(R) sprintf('%.0f W', VbuckSet^2/R), loads, 'UniformOutput', false);
    f  = logspace(0, log10(0.999/(2*Ts)), 4000);
    w  = 2*pi*f;
    fh = fRes*(1:10);
    files = {};

    % Plants (nominal L, C) per load: control path and ripple path.
    P = cell(size(loads)); Pd = P;
    for k = 1:numel(loads)
        R = loads(k);
        G = ss(1/(L*C*s^2 + (L/R + RL*C)*s + 1 + RL/R));        % switch node -> vdc2
        Pk = c2d(Vdc1*G, Ts, 'zoh'); Pk.InputDelay = 1; P{k} = absorbDelay(Pk);
        m0 = (VbuckSet + RL*VbuckSet/R)/Vdc1;
        Pd{k} = c2d(m0*G, Ts, 'zoh');                             % vdc1 -> vdc2, no control
    end

    %% 1 Plant
    fig = newFig('Plant m \rightarrow v_{dc2} (LC, RL = 100 m\Omega, ZOH + 1 sample delay)');
    for k = 1:numel(loads)
        bodeLines(fig, P{k}, w, lnames{k}, '-');
    end
    files{end+1} = finish(fig, f, fh, 'bode_dclink2_1_plant.png');

    %% 2 Controller
    fig = newFig('Controller e \rightarrow m (PI controller 2)');
    for c = 1:size(ctrls, 1)
        bodeLines(fig, ctrls{c, 2}, w, ctrls{c, 1}, '-');
    end
    files{end+1} = finish(fig, f, fh, 'bode_dclink2_2_controller.png');

    %% 3 Open loop
    fig = newFig('Open loop L = C \cdot P');
    styles = {'-', '--'};
    fprintf('\nMargins: margin() reports the worst crossing. With resonant terms the loop crosses\n');
    fprintf('0 dB several times, so fc is that worst crossing; fcPI is the crossover without them.\n');
    fprintf('%-32s %-8s %10s %10s %12s %10s\n', 'controller', 'load', 'GM [dB]', 'PM [deg]', 'fc [Hz]', 'fcPI [Hz]');
    for c = 1:size(ctrls, 1)
        for k = 1:numel(loads)
            Lk = ctrls{c, 2}*P{k};
            bodeLines(fig, Lk, w, [ctrls{c, 1} ', ' lnames{k}], styles{c});
            [gm, pm, ~, wcp] = margin(Lk);
            [~, ~, ~, wpi] = margin(ctrls{c, 3}*P{k});
            fprintf('%-32s %-8s %10.1f %10.1f %12.1f %10.1f\n', ctrls{c, 1}, lnames{k}, 20*log10(gm), pm, ...
                wcp/2/pi, wpi/2/pi);
        end
    end
    files{end+1} = finish(fig, f, fh, 'bode_dclink2_3_openloop.png');

    %% 4 Sensitivity
    fig = figure('Visible', 'off', 'Position', [100 100 1100 600]);
    ax = axes(fig); hold(ax, 'on'); grid(ax, 'on'); set(ax, 'XScale', 'log');
    for c = 1:size(ctrls, 1)
        for k = 1:numel(loads)
            Lk = ctrls{c, 2}*P{k};
            S  = feedback(ss(1, 'Ts', Ts), Lk);
            T  = feedback(Lk, 1);
            semilogx(ax, f, 20*log10(abs(squeeze(freqresp(S, w)))), styles{c}, ...
                'DisplayName', ['|S| ' ctrls{c, 1} ', ' lnames{k}]);
            semilogx(ax, f, 20*log10(abs(squeeze(freqresp(T, w)))), styles{c}, 'LineWidth', 0.5, ...
                'Color', [0.6 0.6 0.6], 'HandleVisibility', ternary(k == 1 && c == 1, 'on', 'off'), ...
                'DisplayName', '|T| (all)');
        end
    end
    yline(ax, 6, 'r:', 'Ms = 6 dB', 'HandleVisibility', 'off');
    xline(ax, fh, ':', 'HandleVisibility', 'off');
    ylim(ax, [-50 15]); xlim(ax, [f(1) f(end)]);
    xlabel(ax, 'f [Hz]'); ylabel(ax, 'magnitude [dB]');
    title(ax, 'Sensitivity S = 1/(1+L) and complementary sensitivity T');
    legend(ax, 'show', 'Location', 'eastoutside');
    files{end+1} = save(fig, 'bode_dclink2_4_sensitivity.png');

    %% 5 Ripple transfer vdc1 -> vdc2
    fig = figure('Visible', 'off', 'Position', [100 100 1100 600]);
    ax = axes(fig); hold(ax, 'on'); grid(ax, 'on'); set(ax, 'XScale', 'log');
    for k = 1:numel(loads)
        semilogx(ax, f, 20*log10(abs(squeeze(freqresp(Pd{k}, w)))), ':', 'LineWidth', 1.5, ...
            'DisplayName', ['no control, ' lnames{k}]);
    end
    set(ax, 'ColorOrderIndex', 1);
    for c = 1:size(ctrls, 1)
        for k = 1:numel(loads)
            S = feedback(ss(1, 'Ts', Ts), ctrls{c, 2}*P{k});
            semilogx(ax, f, 20*log10(abs(squeeze(freqresp(S*Pd{k}, w)))), styles{c}, ...
                'DisplayName', [ctrls{c, 1} ', ' lnames{k}]);
        end
    end
    xline(ax, fh, ':', 'HandleVisibility', 'off');
    xlim(ax, [10 2000]); ylim(ax, [-60 30]);
    xlabel(ax, 'f [Hz]'); ylabel(ax, '|v_{dc2} / v_{dc1}| [dB]');
    title(ax, 'Ripple transfer DC link 1 \rightarrow DC link 2 (harmonics of 50 Hz marked)');
    legend(ax, 'show', 'Location', 'eastoutside');
    files{end+1} = save(fig, 'bode_dclink2_5_ripple.png');

end

function file = finish(fig, f, fh, name)
    for a = findobj(fig, 'Type', 'axes')'
        xline(a, fh, ':', 'HandleVisibility', 'off');
        xlim(a, [f(1) f(end)]);
    end
    legend(findobj(fig, 'Type', 'axes', 'Tag', 'mag'), 'show', 'Location', 'eastoutside');
    file = save(fig, name);
end

function Ct = ctrlOf(Kp, Ki, num, den, A, B, C, D, Ts)
    z = tf('z', Ts);
    Ct = (ss(Kp + Ki*Ts*z/(z - 1)) + ss(A, B, C, D, Ts)) * ss(tf(num, den, Ts, 'Variable', 'z^-1'));
end

function fig = newFig(ttl)
    fig = figure('Visible', 'off', 'Position', [100 100 1100 700]);
    t = tiledlayout(fig, 2, 1, 'TileSpacing', 'compact');
    title(t, ttl);
    am = nexttile(t); hold(am, 'on'); grid(am, 'on'); set(am, 'XScale', 'log', 'Tag', 'mag');
    ylabel(am, 'magnitude [dB]');
    ap = nexttile(t); hold(ap, 'on'); grid(ap, 'on'); set(ap, 'XScale', 'log', 'Tag', 'phase');
    ylabel(ap, 'phase [deg]'); xlabel(ap, 'f [Hz]');
    linkaxes([am ap], 'x');
end

function bodeLines(fig, sys, w, name, style)
    h  = squeeze(freqresp(sys, w));
    am = findobj(fig, 'Type', 'axes', 'Tag', 'mag');
    ap = findobj(fig, 'Type', 'axes', 'Tag', 'phase');
    semilogx(am, w/2/pi, 20*log10(abs(h)), style, 'DisplayName', name);
    semilogx(ap, w/2/pi, rad2deg(angle(h)), style, 'HandleVisibility', 'off');
end

function file = save(fig, name)
    file = fullfile(tempdir, name);
    exportgraphics(fig, file, 'Resolution', 130);
    close(fig);
    fprintf('Plot: %s\n', file);
end

function v = ternary(c, a, b)
    if c, v = a; else, v = b; end
end
