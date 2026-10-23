function generate_control(models)
%GENERATE_CONTROL Generate C code for the control models.
%
%   generate_control() runs a standalone Embedded Coder build (code
%   generation only) for every model in control/models/ and copies the
%   generated code to control/generated/<Model>/. Shared utility code
%   (slprj/ert/_sharedutils) goes to control/generated/shared/.
%
%   generate_control(models) generates only the given models, e.g.
%   generate_control({'Current_Control'}).
%
%   The build settings come from the referenced configuration set in
%   control/data/configuration.sldd. This script does not change them.
%   Build artifacts are written to a temporary folder outside the repository.
%   control/generated/<Model>/ is deleted and recreated on every run, so
%   files that are no longer generated do not remain.
%
%   See AGENTS.md, sections 3.2 and 7 (Phase 3).

    scriptDir    = fileparts(mfilename('fullpath'));
    controlDir   = fileparts(scriptDir);
    modelsDir    = fullfile(controlDir, 'models');
    generatedDir = fullfile(controlDir, 'generated');

    % Files created by the target/board support that are not part of the
    % control code. The CCS projects provide main, ISRs and hardware init.
    excludedFiles = {'ert_main.c', 'MW_*', 'rtmodel.h'};

    if nargin < 1 || isempty(models)
        models = findControlModels(modelsDir);
    end
    models = cellstr(models);

    % Paths needed to load the models and their data dictionaries.
    searchPath = { ...
        modelsDir, ...
        fullfile(controlDir, 'data'), ...
        fullfile(controlDir, 'subsystems')};
    oldPath = addpath(searchPath{:});
    restorePath = onCleanup(@() path(oldPath)); %#ok<NASGU>

    % Keep slprj, the cache and the <Model>_ert_rtw folders out of the repo.
    buildDir = fullfile(tempdir, 'labpsu_control_codegen');
    if isfolder(buildDir)
        rmdir(buildDir, 's');
    end
    mkdir(buildDir);
    oldFileGenCfg = Simulink.fileGenControl('getConfig');
    Simulink.fileGenControl('set', ...
        'CacheFolder',   buildDir, ...
        'CodeGenFolder', buildDir, ...
        'CodeGenFolderStructure', Simulink.filegen.CodeGenFolderStructure.ModelSpecific);
    restoreFileGen = onCleanup(@() Simulink.fileGenControl('setConfig', ...
        'config', oldFileGenCfg)); %#ok<NASGU>

    if ~isfolder(generatedDir)
        mkdir(generatedDir);
    end

    for k = 1:numel(models)
        model = models{k};
        fprintf('### Generating code for %s\n', model);

        wasLoaded = bdIsLoaded(model);
        load_system(model);
        closeModel = onCleanup(@() closeIfLoadedHere(model, wasLoaded));

        slbuild(model, 'GenerateCodeOnly', true);

        rtwDir = fullfile(buildDir, [model '_ert_rtw']);
        if ~isfolder(rtwDir)
            error('generate_control:noCode', ...
                'No generated code found for %s in %s.', model, rtwDir);
        end
        copyCode(rtwDir, fullfile(generatedDir, model), excludedFiles);

        clear closeModel
    end

    % Shared utilities (rtwtypes.h, shared functions and constants), if any.
    sharedUtilsDir = fullfile(buildDir, 'slprj', 'ert', '_sharedutils');
    sharedDir = fullfile(generatedDir, 'shared');
    if isfolder(sharedUtilsDir)
        copyCode(sharedUtilsDir, sharedDir, excludedFiles);
    elseif isfolder(sharedDir)
        rmdir(sharedDir, 's');
    end

    fprintf('### Generated code for %d model(s) in %s\n', numel(models), generatedDir);
end

function models = findControlModels(modelsDir)
    files = dir(fullfile(modelsDir, '*.slx'));
    if isempty(files)
        error('generate_control:noModels', 'No models found in %s.', modelsDir);
    end
    [~, models] = cellfun(@fileparts, {files.name}, 'UniformOutput', false);
    models = sort(models);
end

function copyCode(srcDir, dstDir, excludedFiles)
    if isfolder(dstDir)
        rmdir(dstDir, 's');
    end
    mkdir(dstDir);

    files = [dir(fullfile(srcDir, '*.c')); dir(fullfile(srcDir, '*.h'))];
    for k = 1:numel(files)
        if isExcluded(files(k).name, excludedFiles)
            continue
        end
        copyfile(fullfile(srcDir, files(k).name), fullfile(dstDir, files(k).name));
    end
end

function tf = isExcluded(fileName, patterns)
    tf = false;
    for k = 1:numel(patterns)
        expr = ['^' regexptranslate('wildcard', patterns{k}) '$'];
        if ~isempty(regexp(fileName, expr, 'once'))
            tf = true;
            return
        end
    end
end

function closeIfLoadedHere(model, wasLoaded)
    if ~wasLoaded && bdIsLoaded(model)
        close_system(model, 0);
    end
end
