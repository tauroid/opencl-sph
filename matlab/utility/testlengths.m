function [ output, history ] = testlengths( strainRange, ax, moveStepsPerUnit, stabiliseSteps,...
                                            initialDimensions, particleDimensions,...
                                            totalMass, particleSystem,...
                                            outputParticles, outputVars,...
                                            historyParticles, historyVars )
%TESTLENGTHS Perform strain increments of a cuboid in strainRange along axis
%   Data is recorded for outputVars at outputParticles after the stabilisation
%   period, then the next step is performed
%
%   Axis is 1 for x, 2 for y, 3 for z
%
%   historyVars at historyParticles are recorded for every timestep
%   
%   outputParticles and historyParticles are specified by grid index
%   i.e. [i; j; k] for i in 1:particleDimensions(1),
%   j in 1:particleDimensions(2) and k in 1:particleDimensions(3)

%% Build particle cube
[y, x, z] = meshgrid(linspace(-initialDimensions(1)/2, initialDimensions(1)/2, particleDimensions(1)),...
                     linspace(-initialDimensions(2)/2, initialDimensions(2)/2, particleDimensions(2)),...
                     linspace(-initialDimensions(3)/2, initialDimensions(3)/2, particleDimensions(3)));

xyz = cat(4, x, y, z);

cubeposition = reshape(xyz, [numel(xyz)/3 3])';

cubeposition = single(cubeposition);

particleSystem.init(cubeposition, single(totalMass/prod(particleDimensions)));

% Hack to set the smoothing radius appropriately - adjust if things go
% wrong
%setstate('smoothingradius', single(2.0*( max(initialDimensions ./ (particleDimensions-1) ) )));
fprintf('Smoothing radius: %f\n', querystate('smoothingradius'));
fprintf('Mass: %f\n', querystate('mass'));

%% Get particle indices
[y, x, z] = meshgrid(1:particleDimensions(1), 1:particleDimensions(2), 1:particleDimensions(3));

xyz = cat(4, x, y, z);

if numel(outputParticles) > 0
    outputIndices = reshape(xyz, [numel(xyz)/3 3])';
    [~,~,outputIndices] = intersect(outputParticles', outputIndices', 'rows');
else
    outputIndices = [];
end
if numel(historyParticles) > 0
    historyIndices = reshape(xyz, [numel(xyz)/3 3])';
    [~,~,historyIndices] = intersect(historyParticles', historyIndices', 'rows');
else
    historyIndices = [];
end

%% Choose cube faces to constrain
plane_constraints = single(zeros(3, 2, 2));
plane_constraints(ax, 1, 1) = -initialDimensions(ax)/2;
plane_constraints(ax, 2, 1) = 1;
plane_constraints(ax, 1, 2) = initialDimensions(ax)/2;
plane_constraints(ax, 2, 2) = 1;

plane_constraints_particles = zeros(1, size(cubeposition, 2));

plane_constraints_particles(cubeposition(ax,:) == -initialDimensions(ax)/2) = 1;
plane_constraints_particles(cubeposition(ax,:) == initialDimensions(ax)/2) = 2;

plane_constraints_particles = uint32(plane_constraints_particles);

setstate('plane_constraints', plane_constraints,...
         'plane_constraints_particles', plane_constraints_particles);

%% Set up output variables

output = cell(1, numel(outputVars));

numOutputSamples = numel(strainRange);

for i=1:numel(outputVars)
    outputVar = querystate(outputVars{i});
    
    outputDims = size(outputVar);
    outputDims(end) = numel(outputIndices);
    
    output{i} = zeros([outputDims numOutputSamples]);
end

indexer = {':', ':', ':'}; % If there are arrays with more than 3 dimensions add more colons

    function recordOutput(outputIndex)
        for v=1:numel(outputVars)
            thisFrame = querystate(outputVars{v});
            
            output{v}(indexer{1:ndims(thisFrame)}, outputIndex) = ...
                thisFrame(indexer{1:(ndims(thisFrame)-1)}, outputIndices);
        end
    end

history = cell(1, numel(historyVars));

lowerOrder = flip(find(strainRange < 0));
upperOrder = find(strainRange >= 0);

lowerStrainStepNums = floor(abs(strainRange(lowerOrder))*moveStepsPerUnit);
upperStrainStepNums = floor(strainRange(upperOrder)*moveStepsPerUnit);

numHistorySamples = lowerStrainStepNums(end) + upperStrainStepNums(end) + ...
    numel(strainRange)*stabiliseSteps;

for i=1:numel(historyVars)
    historyVar = querystate(historyVars{i});
    
    historyDims = size(historyVar);
    historyDims(end) = numel(historyIndices);
    
    history{i} = zeros([historyDims numHistorySamples]);
end

historyCounter = 1;

    function failure = recordHistory()
        failure = false;
        
        for v=1:numel(historyVars)
            thisFrame = querystate(historyVars{v});
            
            history{v}(indexer{1:ndims(thisFrame)}, historyCounter) =...
                thisFrame(indexer{1:(ndims(thisFrame)-1)}, historyIndices);
            
            if any(isnan(thisFrame(:))) || (strcmp(historyVars{v}, 'position') && ...
                    any(abs(thisFrame(:)) > 2))
                failure = true;
                warning('Simulation exploded at frame %d', historyCounter);
                
                history{v} = history{v}(indexer{1:ndims(thisFrame)}, 1:historyCounter);
                return;
            end
        end
        
        historyCounter = historyCounter + 1;
    end
     
%% Run the simulation by steps from 0 strain to strainRange lower bound
% Assume that strainRange is sorted low to high

plane1start = -initialDimensions(ax)/2;
plane2start = initialDimensions(ax)/2;

plane1stop = (strainRange(1)+1)*plane1start;
plane2stop = (strainRange(1)+1)*plane2start;

progress = waitbar(0, 'Simulating...');

    function updateProgress()
        waitbar(historyCounter / numHistorySamples);
    end

startStep = 1;
for i=1:numel(lowerOrder)
    for s=startStep:lowerStrainStepNums(i)
        plane_constraints(ax, 1, 1) = (s/lowerStrainStepNums(end))*(plane1stop - plane1start) + plane1start;
        plane_constraints(ax, 1, 2) = (s/lowerStrainStepNums(end))*(plane2stop - plane2start) + plane2start;
        
        setstate('plane_constraints', plane_constraints);
        
        callkernel('apply_plane_constraints');
        
        particleSystem.step();
        
        if recordHistory(); return; end
        
        updateProgress();
    end
    
    startStep = lowerStrainStepNums(i)+1;
    
    plane_constraints(ax, 1, 1) = (strainRange(lowerOrder(i))+1)*plane1start;
    plane_constraints(ax, 1, 2) = (strainRange(lowerOrder(i))+1)*plane2start;
    
    setstate('plane_constraints', plane_constraints);
    
    callkernel('apply_plane_constraints');
    
    for s=1:stabiliseSteps
        callkernel('apply_plane_constraints');
        
        particleSystem.step();
        
        if recordHistory(); return; end
        
        updateProgress();
    end
    
    recordOutput(lowerOrder(i));
end

% Reset
particleSystem.reset();
particleSystem.init(cubeposition, single(totalMass/prod(particleDimensions)));

% Hack to set the smoothing radius appropriately - adjust if things go
% wrong
%setstate('smoothingradius', single(2.5*( max(initialDimensions ./ (particleDimensions-1) ) )));

setstate('plane_constraints', plane_constraints,...
         'plane_constraints_particles', plane_constraints_particles);

%% Run the simulation by steps from 0 strain to strainRange upper bound

plane1stop = (strainRange(end)+1)*plane1start;
plane2stop = (strainRange(end)+1)*plane2start;

startStep = 1;
for i=1:numel(upperOrder)
    for s=startStep:upperStrainStepNums(i)
        plane_constraints(ax, 1, 1) = (s/upperStrainStepNums(end))*(plane1stop - plane1start) + plane1start;
        plane_constraints(ax, 1, 2) = (s/upperStrainStepNums(end))*(plane2stop - plane2start) + plane2start;
        
        setstate('plane_constraints', plane_constraints);
        
        callkernel('apply_plane_constraints');
        
        particleSystem.step();
        
        if recordHistory(); return; end
        
        updateProgress();
    end
    
    startStep = upperStrainStepNums(i)+1;
    
    plane_constraints(ax, 1, 1) = (strainRange(upperOrder(i))+1)*plane1start;
    plane_constraints(ax, 1, 2) = (strainRange(upperOrder(i))+1)*plane2start;
    
    setstate('plane_constraints', plane_constraints);
    
    callkernel('apply_plane_constraints');
    
    for s=1:stabiliseSteps
        callkernel('apply_plane_constraints');
        
        particleSystem.step();
        
        if recordHistory(); return; end
        
        updateProgress();
    end
    
    recordOutput(upperOrder(i));
end

close(progress);
end

