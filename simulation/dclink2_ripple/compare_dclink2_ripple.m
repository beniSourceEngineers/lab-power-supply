function results = compare_dclink2_ripple()
%COMPARE_DCLINK2_RIPPLE DC link 2 ripple: PI only, previous and new controller.
%
%   Runs dclink2_ripple.slx with the harmonic DC link 1 ripple of
%   dclink1_ripple.m (amplitude and phase depend on the operating point) for
%     - steady state at 3 kW, 900 W, 300 W and 30 W (load 30..3000 ohm),
%     - 3 kW with the grid at 49.8 Hz (resonant terms stay at 50 Hz multiples),
%     - a load step 300 W -> 3 kW at t = 0.6 s (ripple spectrum changes too),
%   each with three controllers:
%     PI only           new Kp, Ki and low-pass, no resonant terms
%     previous          Ki 0.3, notch, resonant terms (dclink2_previous_controller.m)
%     new               Kp 1e-3, Ki 2, low-pass, resonant terms (parameters)
%   all on the plant with RL from the parameters (100 mOhm).
%   Steady state is evaluated over the last 0.4 s: peak-to-peak and RMS of the
%   DC link 2 ripple, and its amplitude per harmonic. For the load step: the
%   largest deviation from VbuckSet after the step and the RMS ripple in
%   0.6..0.7 s. Plots are saved to <tempdir>/compare_dclink2_ripple*.png.

    here  = fileparts(mfilename('fullpath'));
    model = 'dclink2_ripple';
    oldPath = addpath(here);
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>
    build_dclink2_ripple();
    load_system(model);
    run(fullfile(here, 'dclink2_ripple_params.m'));

    % Controller variants: variables set in the model workspace.
    prev = dclink2_previous_controller(Ts, L, C, RL, Rnom, Vdc1); %#ok<NODEF>
    ctrl = struct( ...
        'name', {'PI only (new Kp, Ki)', 'previous (Ki 0.3, notch)', 'new (Kp, Ki 2, low-pass)'}, ...
        'vars', {{'resEnable', 0}, ...
                 {'Kp', prev.Kp; 'Ki', prev.Ki; 'filtNum', prev.filtNum; 'filtDen', prev.filtDen; ...
                  'resA', prev.resA; 'resB', prev.resB; 'resC', prev.resC; 'resD', prev.resD}, ...
                 cell(0, 2)});

    ops = struct( ...
        'name', {'3 kW', '900 W', '300 W', '30 W', '3 kW, grid 49.8 Hz', 'step 300 W -> 3 kW'}, ...
        'R1', {Rnom, 100, 300, 3000, Rnom, 300}, ...
        'R2', {Rnom, 100, 300, 3000, Rnom, Rnom}, ...
        'fg', {50, 50, 50, 50, 49.8, 50}, ...
        'step', {false, false, false, false, false, true});

    stopTime = 1.2;
    results = struct('op', {}, 'ctrl', {}, 'pp', {}, 'rms', {}, 'harm', {}, 'dev', {}, 'v', {});
    for o = 1:numel(ops)
        for c = 1:numel(ctrl)
            in = Simulink.SimulationInput(model);
            in = in.setModelParameter('StopTime', num2str(stopTime));
            P1 = VbuckSet^2/ops(o).R1;
            P2 = VbuckSet^2/ops(o).R2;
            [a1, p1] = dclink1_ripple(P1, Pnom);
            [a2, p2] = dclink1_ripple(P2, Pnom);
            vars = [{'Rload1', ops(o).R1; 'Rload2', ops(o).R2; 'fg', ops(o).fg; ...
                     'ripAmp1', a1; 'ripPh1', p1; 'ripAmp2', a2; 'ripPh2', p2; ...
                     'iL0', VbuckSet/ops(o).R1; 'm0', (VbuckSet + RL*VbuckSet/ops(o).R1)/Vdc1}; ...
                    ctrl(c).vars];
            if ~ops(o).step
                vars = [vars; {'tStep', 10*stopTime}]; %#ok<AGROW>
            end
            for k = 1:size(vars, 1)
                in = in.setVariable(vars{k, 1}, vars{k, 2}, 'Workspace', model);
            end
            out = sim(in);
            v = out.logsout.get('vdc2').Values;

            r.op = ops(o).name;
            r.ctrl = ctrl(c).name;
            r.v = v;
            if ops(o).step
                win = getsampleusingtime(v, tStep, tStep + 0.1);
                r.dev = max(abs(win.Data - VbuckSet));
                x = uniform(win);
                r.rms = rms(x - mean(x));
                r.pp = max(win.Data) - min(win.Data);
                r.harm = nan(1, 10);
            else
                win = getsampleusingtime(v, stopTime - 0.4, stopTime);
                x = uniform(win);
                r.pp = max(win.Data) - min(win.Data);
                r.rms = rms(x - mean(x));
                r.harm = arrayfun(@(h) tone(win, h*ops(o).fg), 1:10);
                r.dev = NaN;
            end
            results(end+1) = r; %#ok<AGROW>
        end
    end

    fprintf('\nDC link 2 ripple (VbuckSet %g V). DC link 1 ripple: harmonics of the grid, dclink1_ripple.m\n', VbuckSet);
    fprintf('%-20s %-26s %9s %9s   amplitude per harmonic 1..10 [V]\n', 'operating point', 'controller', 'pp [V]', 'rms [V]');
    for r = results
        if all(isnan(r.harm))
            fprintf('%-20s %-26s %9.3f %9.3f   max deviation after step: %.2f V\n', r.op, r.ctrl, r.pp, r.rms, r.dev);
        else
            fprintf('%-20s %-26s %9.3f %9.3f  ', r.op, r.ctrl, r.pp, r.rms);
            fprintf(' %5.2f', r.harm); fprintf('\n');
        end
    end

    plotCase(results, '3 kW', stopTime - 0.1, 0.1, VbuckSet, 'compare_dclink2_ripple.png');
    plotCase(results, 'step 300 W -> 3 kW', tStep - 0.05, 0.25, VbuckSet, 'compare_dclink2_ripple_step.png');
end

function x = uniform(ts)
    t = linspace(ts.Time(1), ts.Time(end), 50000)';
    x = interp1(ts.Time, double(squeeze(ts.Data)), t);
end

function a = tone(ts, f)
    % Amplitude at f over whole periods, on a uniform grid (variable-step data).
    t0 = ts.Time(1);
    n = floor((ts.Time(end) - t0)*f);
    t = linspace(t0, t0 + n/f, 2000*n + 1)';
    x = interp1(ts.Time, double(squeeze(ts.Data)), t);
    x = x(1:end-1) - mean(x(1:end-1));
    t = t(1:end-1);
    a = 2*abs(sum(x .* exp(-1j*2*pi*f*t)))/numel(x);
end

function plotCase(results, op, t0, span, Vset, fileName)
    sel = results(strcmp({results.op}, op));
    fig = figure('Visible', 'off', 'Position', [100 100 1100 420]);
    hold on; grid on;
    for r = sel
        plot(r.v.Time, squeeze(r.v.Data), 'DisplayName', r.ctrl);
    end
    xlim(t0 + [0 span]); ylim(Vset + [-12 12]);
    xlabel('t [s]'); ylabel('V_{dc link 2} [V]');
    title(sprintf('DC link 2, %s, harmonic ripple on DC link 1', op));
    legend('show', 'Location', 'eastoutside');
    file = fullfile(tempdir, fileName);
    exportgraphics(fig, file, 'Resolution', 150);
    close(fig);
    fprintf('Plot: %s\n', file);
end
