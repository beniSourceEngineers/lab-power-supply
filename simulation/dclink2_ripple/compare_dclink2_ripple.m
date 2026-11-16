function results = compare_dclink2_ripple()
%COMPARE_DCLINK2_RIPPLE DC link 2 ripple with PI only and with PI + resonant.
%
%   Runs dclink2_ripple.slx for these cases:
%     - full load (Rload from the parameters) and light load (10x Rload),
%     - ripple exactly at the resonant frequency and 0.5 Hz off (grid
%       frequency deviation; the resonant term stays tuned to fRip),
%   each with the resonant term off (PI only) and on.
%   Evaluated in steady state (t >= 0.6 s): peak-to-peak of DC link 2 and the
%   amplitude of its component at the ripple frequency. A plot is saved to
%   <tempdir>/compare_dclink2_ripple.png.

    here  = fileparts(mfilename('fullpath'));
    model = 'dclink2_ripple';
    oldPath = addpath(here);
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>
    if ~isfile(fullfile(here, [model '.slx']))
        build_dclink2_ripple();
    end
    load_system(model);

    p = struct();
    run(fullfile(here, 'dclink2_ripple_params.m'));
    p.Rload = Rload; p.fRip = fRip; p.Vdc1Rip = Vdc1Rip; %#ok<NODEF>

    cases = { ...
        'full load',             p.Rload,    p.fRip; ...
        'light load',            10*p.Rload, p.fRip; ...
        'full load, ripple -0.5 Hz', p.Rload, p.fRip - 0.5};
    tStart = 0.6;

    results = struct('name', {}, 'resEnable', {}, 'fRip', {}, 'vPp', {}, 'vAmp', {}, ...
        'mMin', {}, 'mMax', {}, 'vdc2', {});
    for c = 1:size(cases, 1)
        for res = [0 1]
            in = Simulink.SimulationInput(model);
            in = in.setVariable('Rload', cases{c, 2}, 'Workspace', model);
            in = in.setVariable('fRip', cases{c, 3}, 'Workspace', model);
            in = in.setVariable('resEnable', res, 'Workspace', model);
            out = sim(in);

            v = steady(out.logsout.get('vdc2').Values, tStart);
            m = steady(out.logsout.get('m').Values, tStart);
            r.name = cases{c, 1};
            r.resEnable = res;
            r.fRip = cases{c, 3};
            r.vPp = max(v.Data) - min(v.Data);
            r.vAmp = tone(v, cases{c, 3});
            r.mMin = min(m.Data);
            r.mMax = max(m.Data);
            r.vdc2 = v;
            results(end+1) = r; %#ok<AGROW>
        end
    end

    fprintf('\nDC link 1: ripple %g V amplitude at %g Hz\n', p.Vdc1Rip, p.fRip);
    fprintf('%-26s %-14s %12s %14s %14s\n', 'case', 'controller', 'Vdc2 pp [V]', 'Vdc2 @f [V]', 'm range');
    for r = results
        if r.resEnable, ctl = 'PI + resonant'; else, ctl = 'PI only'; end
        fprintf('%-26s %-14s %12.3f %14.4f %7.3f..%.3f\n', r.name, ctl, r.vPp, r.vAmp, r.mMin, r.mMax);
    end
    for c = 1:size(cases, 1)
        sel = results(strcmp({results.name}, cases{c, 1}));
        fprintf('%s: ripple at f reduced by %.1f dB\n', cases{c, 1}, 20*log10(sel(1).vAmp/sel(2).vAmp));
    end

    fig = figure('Visible', 'off', 'Position', [100 100 1100 320*size(cases, 1)]);
    for c = 1:size(cases, 1)
        sel = results(strcmp({results.name}, cases{c, 1}));
        subplot(size(cases, 1), 1, c); hold on; grid on;
        for r = sel
            if r.resEnable, lbl = 'PI + resonant'; else, lbl = 'PI only'; end
            plot(r.vdc2.Time, squeeze(r.vdc2.Data), 'DisplayName', lbl);
        end
        xlim(tStart + [0 0.1]); ylabel('V_{dc link 2} [V]'); xlabel('t [s]');
        title(sprintf('%s, %g V / %g Hz ripple on DC link 1', cases{c, 1}, p.Vdc1Rip, cases{c, 3}));
        legend('show', 'Location', 'eastoutside');
    end
    file = fullfile(tempdir, 'compare_dclink2_ripple.png');
    exportgraphics(fig, file, 'Resolution', 150);
    close(fig);
    fprintf('Plot: %s\n', file);
end

function ts = steady(ts, tStart)
    ts = getsampleusingtime(ts, tStart, ts.Time(end));
end

function a = tone(ts, f)
    % Amplitude at f over whole periods, on a uniform grid (variable-step data).
    t0 = ts.Time(1);
    n = floor((ts.Time(end) - t0)*f);
    t = linspace(t0, t0 + n/f, 20000*n + 1)';
    x = interp1(ts.Time, double(squeeze(ts.Data)), t);
    x = x(1:end-1) - mean(x(1:end-1));
    t = t(1:end-1);
    a = 2*abs(sum(x .* exp(-1j*2*pi*f*t)))/numel(x);
end
