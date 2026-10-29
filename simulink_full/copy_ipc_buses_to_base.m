function copy_ipc_buses_to_base()
%COPY_IPC_BUSES_TO_BASE Copy IPC bus definitions from ipc.sldd to the base workspace.
%   Copies com2ctrl and ctrl2com (and any nested Bus types they reference
%   that are defined in ipc.sldd) into the base workspace under the same names.

ddPath = fullfile(fileparts(fileparts(mfilename('fullpath'))), 'data', 'ipc.sldd');
dd = Simulink.data.dictionary.open(ddPath);
ddSec = getSection(dd, 'Design Data');

pending = {'com2ctrl', 'ctrl2com'};
copied = {};
while ~isempty(pending)
    name = pending{1};
    pending(1) = [];
    if any(strcmp(copied, name))
        continue
    end
    busObj = getValue(getEntry(ddSec, name));
    assignin('base', name, busObj);
    copied{end+1} = name; %#ok<AGROW>

    % Queue nested bus types (e.g. 'Bus: subBus') defined in the dictionary
    for el = busObj.Elements'
        dt = strtrim(regexprep(el.DataType, '^Bus:\s*', ''));
        if ~strcmp(dt, el.DataType) || (exist(ddSec, dt) && isa(getValue(getEntry(ddSec, dt)), 'Simulink.Bus'))
            pending{end+1} = dt; %#ok<AGROW>
        end
    end
end

close(dd);
fprintf('Copied to base workspace: %s\n', strjoin(copied, ', '));
end
