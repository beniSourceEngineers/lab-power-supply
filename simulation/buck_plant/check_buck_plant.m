function check_buck_plant()
%CHECK_BUCK_PLANT Verify buck_plant.slx against the transfer function and demo Vin ripple.
%
%   1. For every operating point Vout x Pout (constant Vin = 400 V): steady
%      state and the response to a small step in m compared with
%          Vin*Rload/((Rload*L*C)*s^2 + (L + Rload*Rseries*C)*s + Rload + Rseries),
%      plus a DCM check (Lcrit = Rload*(1 - m)/(2*fsw) must be below L).
%   2. Demo: Vin with the DC link 1 harmonics from the scope photo, constant m.
%   Plots: <tempdir>/check_buck_plant*.png. Base MATLAB + Simulink.

    here = fileparts(mfilename('fullpath'));
    oldPath = addpath(here);
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>
    model = 'buck_plant';
    if ~isfile(fullfile(here, [model '.slx'])), build_buck_plant(); end
    load_system(model);
    p = loadParams(fullfile(here, 'buck_plant_params.m'));

    Vin  = 400;
    fsw  = 2*58e3;                        % Hz, buck switching frequency (Beni 2026-11-18)
    Vout = [100 150 200 250 300 400];
    Pout = [1 50 200 400];
    tStep = 0.02; tEnd = 0.04; dm = 0.01;

    fprintf('%6s %6s %8s %7s %9s %9s %12s %s\n', 'Vout', 'Pout', 'Rload', 'm0', 'Lcrit', 'vout ss', 'step error', 'note');
    fig = figure('Visible', 'off', 'Position', [100 100 1100 700]); tiledlayout(fig, 'flow');
    for i = 1:numel(Vout)
        for j = 1:numel(Pout)
            R  = Vout(i)^2/Pout(j);
            m0 = Vout(i)*(R + p.Rseries)/(R*Vin);
            Lcrit = R*(1 - m0)/(2*fsw);
            note = '';
            if m0 > 0.95
                fprintf('%6g %6g %8.1f %7.3f %9s %9s %12s %s\n', Vout(i), Pout(j), R, m0, '-', '-', '-', ...
                    'not reachable (m > 0.95)');
                continue
            end
            if Lcrit > p.L, note = sprintf('DCM (Lcrit %.0f uH > L): model not valid', 1e6*Lcrit); end

            t  = (0:1e-5:tEnd)';
            m  = m0 + dm*(t >= tStep);
            in = Simulink.SimulationInput(model);
            in = in.setModelParameter('StopTime', num2str(tEnd), 'LoadExternalInput', 'on', ...
                'ExternalInput', '[t, m, vin, vout, pout]', 'SaveOutput', 'on', 'OutputSaveName', 'yout', ...
                'SaveFormat', 'Array', 'SaveTime', 'on', 'TimeSaveName', 'tout');
            in = in.setVariable('t', t); in = in.setVariable('m', m);
            in = in.setVariable('vin', Vin*ones(size(t)));
            in = in.setVariable('vout', Vout(i)*ones(size(t)));
            in = in.setVariable('pout', Pout(j)*ones(size(t)));
            in = in.setVariable('vC0', Vout(i), 'Workspace', model);
            in = in.setVariable('iL0', Vout(i)/R, 'Workspace', model);
            out = sim(in);
            y = interp1(out.tout, out.yout(:, 1), t);

            % Transfer function step response (base MATLAB: exact ZOH of the state model)
            A = [-p.Rseries/p.L, -1/p.L; 1/p.C, -1/(R*p.C)];
            B = [Vin/p.L; 0];
            dt = t(2) - t(1); Ad = expm(A*dt); Bd = A\((Ad - eye(2))*B);
            x = zeros(2, 1); yTf = zeros(size(t));
            for k = 1:numel(t)
                yTf(k) = x(2); x = Ad*x + Bd*(m(k) - m0);
            end
            yTf = Vout(i) + yTf;
            err = max(abs(y(t > tStep) - yTf(t > tStep)));
            fprintf('%6g %6g %8.1f %7.3f %7.0fuH %9.2f %10.2e V %s\n', Vout(i), Pout(j), R, m0, ...
                1e6*Lcrit, y(find(t < tStep, 1, 'last')), err, note);
            if j == numel(Pout) || j == 1
                nexttile; plot(1e3*t, y, 1e3*t, yTf, '--'); grid on; xlim(1e3*[tStep - 0.002, tEnd]);
                title(sprintf('%g V, %g W', Vout(i), Pout(j))); xlabel('t [ms]'); ylabel('vout [V]');
            end
        end
    end
    ax = findobj(fig, 'Type', 'axes');
    legend(ax(1), {'buck\_plant.slx', 'transfer function'}, 'Location', 'southeast');
    exportgraphics(fig, fullfile(tempdir, 'check_buck_plant_steps.png'), 'Resolution', 110); close(fig);

    %% Demo: Vin with the measured DC link 1 ripple, 300 V / 400 W, constant m
    ampNom    = [1.77 0.71 0.09 0.06 0.06 0.03 0.01 0.01 0.01 0.02];
    basePhase = [-1.49 1.25 2.22 -1.65 -1.52 1.78 0.11 0.98 -1.66 0.33];
    t   = (0:1e-5:0.1)';
    vin = Vin + sum(ampNom .* sin(2*pi*50*t*(1:10) + basePhase), 2);
    R   = 300^2/400; m0 = 300*(R + p.Rseries)/(R*Vin);
    in = Simulink.SimulationInput(model);
    in = in.setModelParameter('StopTime', '0.1', 'LoadExternalInput', 'on', ...
        'ExternalInput', '[t, m, vin, vout, pout]', 'SaveOutput', 'on', 'OutputSaveName', 'yout', ...
        'SaveFormat', 'Array', 'SaveTime', 'on', 'TimeSaveName', 'tout');
    in = in.setVariable('t', t); in = in.setVariable('m', m0*ones(size(t)));
    in = in.setVariable('vin', vin); in = in.setVariable('vout', 300*ones(size(t)));
    in = in.setVariable('pout', 400*ones(size(t)));
    in = in.setVariable('vC0', 300, 'Workspace', model);
    in = in.setVariable('iL0', 300/R, 'Workspace', model);
    out = sim(in);
    fig = figure('Visible', 'off', 'Position', [100 100 1000 450]);
    yyaxis left;  plot(t, vin); ylabel('V_{in} (DC link 1) [V]');
    yyaxis right; plot(out.tout, out.yout(:, 1), out.tout, out.yout(:, 2), '--'); ylabel('V_{out} (DC link 2) [V]');
    legend('V_{in}', 'v_{out}', 'v_{out} after RC filter'); grid on; xlabel('t [s]');
    title(sprintf('buck\\_plant.slx, 300 V / 400 W, constant m = %.3f, measured DC link 1 ripple', m0));
    exportgraphics(fig, fullfile(tempdir, 'check_buck_plant_ripple.png'), 'Resolution', 110); close(fig);
    sel = out.tout > 0.05;
    fprintf('\nRipple demo: Vin %.2f V pp -> vout %.2f V pp (open loop, m constant)\n', ...
        max(vin) - min(vin), max(out.yout(sel, 1)) - min(out.yout(sel, 1)));
    fprintf('Plots: %s, %s\n', fullfile(tempdir, 'check_buck_plant_steps.png'), ...
        fullfile(tempdir, 'check_buck_plant_ripple.png'));
end

function p = loadParams(file)
    run(file);
    p = struct('L', L, 'C', C, 'Rseries', Rseries);
end
