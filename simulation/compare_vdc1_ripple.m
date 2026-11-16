function results = compare_vdc1_ripple(rippleFreqs, rippleRel)
%COMPARE_VDC1_RIPPLE Ripple on DC link 2 with and without V_dc1 feedforward.
%
%   results = compare_vdc1_ripple() runs SystemSimulation.slx with a sine
%   ripple on DC link 1 (the buck input, PFC line ripple) and compares the
%   ripple of the output voltage (DC link 2) and of the inductor current
%   with the input-voltage feedforward of Current_Control off and on.
%
%   "Off" feeds the nominal DC link voltage to the controller, which gives
%   exactly the controller without feedforward. "On" feeds the measured
%   DC link voltage including the ripple.
%
%   compare_vdc1_ripple(rippleFreqs, rippleRel) sets the ripple frequencies
%   in Hz (default [100 50]) and the ripple amplitude relative to the
%   nominal DC link voltage (default 0.025, i.e. +-10 V on 400 V).
%
%   The ripple is evaluated in steady state (t >= 4 s): peak-to-peak value
%   and the amplitude of the component at the ripple frequency. A plot is
%   saved to <tempdir>/compare_vdc1_ripple.png.

    if nargin < 1 || isempty(rippleFreqs), rippleFreqs = [100 50]; end
    if nargin < 2 || isempty(rippleRel),   rippleRel = 0.025;      end

    simDir  = fileparts(mfilename('fullpath'));
    rootDir = fileparts(simDir);
    oldPath = addpath(simDir, ...
        fullfile(rootDir, 'control', 'models'), ...
        fullfile(rootDir, 'control', 'data'), ...
        fullfile(rootDir, 'control', 'subsystems'), ...
        fullfile(rootDir, 'control', 'plant'), ...
        fullfile(rootDir, 'simulink_full'), ...
        fullfile(rootDir, 'simulink_full', 'data'));
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>

    model = 'SystemSimulation';
    load_system(model);
    tStart = 4;

    % The plant runs from Vin (plant.sldd); the ripple source uses
    % sim_vdcNominal from the model workspace. They must agree.
    dd = Simulink.data.dictionary.open(fullfile(rootDir, 'control', 'plant', 'plant.sldd'));
    vin = getValue(getEntry(getSection(dd, 'Design Data'), 'Vin'));
    close(dd);
    vdcNominal = getVariable(get_param(model, 'ModelWorkspace'), 'sim_vdcNominal');
    assert(vin == vdcNominal, 'plant Vin (%g V) and sim_vdcNominal (%g V) differ', vin, vdcNominal);

    results = struct('freq', {}, 'feedforward', {}, 'voutPp', {}, 'voutAmp', {}, ...
        'iLPp', {}, 'iLAmp', {}, 'dutyMax', {}, 'vout', {}, 'iL', {});
    for f = rippleFreqs
        for ff = [0 1]
            in = Simulink.SimulationInput(model);
            in = in.setVariable('sim_rippleFreq', f, 'Workspace', model);
            in = in.setVariable('sim_rippleRel', rippleRel, 'Workspace', model);
            in = in.setVariable('sim_vdcFeedforward', ff, 'Workspace', model);
            out = sim(in);

            vout = steady(signal(out.logsout, 'Vout', ''), tStart);
            iL   = steady(signal(out.logsout, 'iL', ''), tStart);
            duty = steady(signal(out.logsout, '', [model '/Bus Selector']), tStart);

            r.freq = f;
            r.feedforward = ff;
            r.voutPp = max(vout.Data) - min(vout.Data);
            r.voutAmp = toneAmplitude(vout, f);
            r.iLPp = max(iL.Data) - min(iL.Data);
            r.iLAmp = toneAmplitude(iL, f);
            r.dutyMax = max(duty.Data);
            r.vout = vout;
            r.iL = iL;
            results(end+1) = r; %#ok<AGROW>
        end
    end

    fprintf('\nDC link 1: %g V nominal, ripple +-%.3g V (%.3g %%)\n', ...
        vdcNominal, vdcNominal*rippleRel, 100*rippleRel);
    fprintf('%8s %12s %14s %14s %14s %14s %9s\n', 'f [Hz]', 'feedforward', ...
        'Vout pp [mV]', 'Vout@f [mV]', 'iL pp [mA]', 'iL@f [mA]', 'max duty');
    for r = results
        fprintf('%8g %12s %14.3f %14.3f %14.3f %14.3f %9.3f\n', r.freq, ...
            onOff(r.feedforward), 1e3*r.voutPp, 1e3*r.voutAmp, 1e3*r.iLPp, 1e3*r.iLAmp, r.dutyMax);
    end
    for f = rippleFreqs
        off = results([results.freq] == f & [results.feedforward] == 0);
        on  = results([results.freq] == f & [results.feedforward] == 1);
        fprintf('%g Hz: Vout ripple at f reduced by %.1f dB\n', f, 20*log10(off.voutAmp/on.voutAmp));
    end

    plotResults(results, rippleFreqs);
end

function ts = signal(logsout, name, blockPath)
    for k = 1:logsout.numElements
        e = logsout.getElement(k);
        if (~isempty(name) && strcmp(e.Name, name)) || ...
           (~isempty(blockPath) && strcmp(e.BlockPath.getBlock(1), blockPath))
            ts = e.Values;
            return
        end
    end
    error('compare_vdc1_ripple:noSignal', 'Logged signal %s%s not found.', name, blockPath);
end

function ts = steady(ts, tStart)
    ts = getsampleusingtime(ts, tStart, ts.Time(end));
end

function a = toneAmplitude(ts, f)
    % Amplitude of the component at f over whole periods (single DFT bin).
    t = ts.Time - ts.Time(1);
    x = double(squeeze(ts.Data));
    n = find(t <= floor(t(end)*f)/f, 1, 'last');
    t = t(1:n); x = x(1:n) - mean(x(1:n));
    a = 2*abs(sum(x .* exp(-1j*2*pi*f*t)))/n;
end

function s = onOff(ff)
    if ff, s = 'on'; else, s = 'off'; end
end

function plotResults(results, rippleFreqs)
    fig = figure('Visible', 'off', 'Position', [100 100 1000 300*numel(rippleFreqs)]);
    for i = 1:numel(rippleFreqs)
        f = rippleFreqs(i);
        sel = results([results.freq] == f);
        subplot(numel(rippleFreqs), 2, 2*i-1); hold on; grid on;
        for r = sel
            plot(r.vout.Time, squeeze(r.vout.Data), 'DisplayName', ['feedforward ' onOff(r.feedforward)]);
        end
        xlim(sel(1).vout.Time(1) + [0 4/f]); xlabel('t [s]'); ylabel('Vout [V]');
        title(sprintf('DC link 2 voltage, %g Hz ripple on DC link 1', f)); legend show;
        subplot(numel(rippleFreqs), 2, 2*i); hold on; grid on;
        for r = sel
            plot(r.iL.Time, squeeze(r.iL.Data), 'DisplayName', ['feedforward ' onOff(r.feedforward)]);
        end
        xlim(sel(1).iL.Time(1) + [0 4/f]); xlabel('t [s]'); ylabel('iL [A]');
        title(sprintf('Inductor current, %g Hz ripple', f)); legend show;
    end
    file = fullfile(tempdir, 'compare_vdc1_ripple.png');
    exportgraphics(fig, file, 'Resolution', 150);
    close(fig);
    fprintf('Plot: %s\n', file);
end
