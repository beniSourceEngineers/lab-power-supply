function build_buck_plant()
%BUILD_BUCK_PLANT Create buck_plant.slx: discrete averaged buck with runtime Vin.
%
%   Inports  1 m      modulation index (duty cycle), 0..1
%            2 Vin    DC link 1 voltage (V), may vary at runtime (ripple)
%            3 Vout   operating point: DC link 2 voltage (V) that defines the load
%            4 Pout   operating point: output power (W);   Rload = Vout^2/Pout
%   Outports 1 vout      DC link 2 voltage (V), unfiltered
%            2 voutMeas  vout after the analog RC filter (what the ADC sees)
%            3 iL        inductor current (A)
%
%   The plant is linear in the switch-node voltage m*Vin, so a varying Vin
%   is modelled exactly (no linearisation around a fixed Vin). For constant
%   Vin, Vout, Pout the transfer function m -> vout equals
%       Vin*Rload/((Rload*L*C)*s^2 + (L + Rload*Rseries*C)*s + Rload + Rseries).
%   The load is a conductance Pout/Vout^2 computed at runtime, so Vout and
%   Pout can also change during a simulation (load steps).
%   Discrete: one Discrete State-Space block (exact ZOH at TsPlant, states
%   [iL; vC; vMeas]); the load current is fed back from vC(k), which adds a
%   one-sample delay in the load path (error checked by check_buck_plant.m).
%   Parameters: buck_plant_params.m (model workspace). Usable stand-alone or
%   in a Model block.

    here  = fileparts(mfilename('fullpath'));
    model = 'buck_plant';
    file  = fullfile(here, [model '.slx']);
    if bdIsLoaded(model), close_system(model, 0); end
    if isfile(file), delete(file); end

    new_system(model);
    cleanup = onCleanup(@() close_system(model, 0));
    set_param(model, 'SolverType', 'Fixed-step', 'Solver', 'FixedStepDiscrete', 'FixedStep', 'TsPlant', ...
        'StopTime', '0.1');

    mw = get_param(model, 'ModelWorkspace');
    mw.DataSource = 'MATLAB File';
    mw.FileName = 'buck_plant_params.m';

    b = @(lib, name, pos, varargin) add_block(lib, [model '/' name], 'Position', pos, varargin{:});
    l = @(src, dst) add_line(model, src, dst, 'autorouting', 'on');

    b('simulink/Sources/In1', 'm',    [40 53 70 67],   'Port', '1');
    b('simulink/Sources/In1', 'Vin',  [40 103 70 117], 'Port', '2');
    b('simulink/Sources/In1', 'Vout', [40 213 70 227], 'Port', '3');
    b('simulink/Sources/In1', 'Pout', [40 263 70 277], 'Port', '4');
    % Hold inputs between samples (m comes from a sampled controller)
    for n = {'m', 'Vin', 'Vout', 'Pout'}
        set_param([model '/' n{1}], 'Interpolate', 'off');
    end

    b('simulink/Math Operations/Product', 'Switch node', [130 60 160 110], 'Inputs', '**');
    set_param([model '/Switch node'], 'Description', 'Averaged switch-node voltage m*Vin');

    % Load conductance Pout/Vout^2 (guarded against Vout = 0)
    b('simulink/Math Operations/Math Function', 'Vout^2', [110 205 150 235], 'Operator', 'square');
    b('simulink/Discontinuities/Saturation', 'Vout^2 min', [180 205 210 235], ...
        'UpperLimit', 'inf', 'LowerLimit', '1');
    b('simulink/Math Operations/Product', 'Gload', [250 220 280 270], 'Inputs', '*/');
    set_param([model '/Gload'], 'Description', 'Load conductance 1/Rload = Pout/Vout^2');
    b('simulink/Math Operations/Product', 'iLoad', [330 170 360 220], 'Inputs', '**');

    b('simulink/Signal Routing/Mux', 'Mux', [400 70 405 160], 'Inputs', '2');
    b('simulink/Discrete/Discrete State-Space', 'LC + RC filter', [440 95 560 135], ...
        'A', 'Ad', 'B', 'Bd', 'C', 'Cd', 'D', 'Dd', 'InitialCondition', '[iL0; vC0; vC0]', ...
        'SampleTime', 'TsPlant');
    set_param([model '/LC + RC filter'], 'Description', ...
        'x = [iL; vC; vMeas], u = [m*Vin; iLoad], exact ZOH at TsPlant (buck_plant_params.m)');
    b('simulink/Signal Routing/Demux', 'Demux', [590 95 595 155], 'Outputs', '3');

    b('simulink/Sinks/Out1', 'vout',     [820 113 850 127], 'Port', '1');
    b('simulink/Sinks/Out1', 'voutMeas', [820 173 850 187], 'Port', '2');
    b('simulink/Sinks/Out1', 'iL',       [820 63 850 77],   'Port', '3');

    l('m/1', 'Switch node/1');
    l('Vin/1', 'Switch node/2');
    l('Switch node/1', 'Mux/1');
    l('Vout/1', 'Vout^2/1');
    l('Vout^2/1', 'Vout^2 min/1');
    l('Pout/1', 'Gload/1');
    l('Vout^2 min/1', 'Gload/2');
    l('Gload/1', 'iLoad/2');
    l('Demux/2', 'iLoad/1');
    l('iLoad/1', 'Mux/2');
    l('Mux/1', 'LC + RC filter/1');
    l('LC + RC filter/1', 'Demux/1');
    l('Demux/1', 'iL/1');
    l('Demux/2', 'vout/1');
    l('Demux/3', 'voutMeas/1');

    a = Simulink.Annotation([model '/note']);
    a.Text = sprintf(['Averaged buck (CCM). Vin can vary at runtime (DC link 1 ripple).\n' ...
        'Discrete state space at TsPlant. Load: Rload = Vout^2/Pout from the inputs. Parameters: buck_plant_params.m']);
    a.Position = [40 320 700 360];

    save_system(model, file);
    fprintf('Saved %s\n', file);
end
