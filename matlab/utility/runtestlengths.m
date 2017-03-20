function [ forces, positions ] = runtestlengths(particles_on_edge, strains, particle_system)
    % RUNTESTLENGTHS Test particle cube with particles_on_edge^3 particles, at the given
    % strains.
    %   With particles at opposing faces constrained to two moving
    %   planes, but free to move within the plane.
    %   Outputs sum of forces on one of the constrained planes for each strain,
    %   and optionally the position history for playback.
    p_edge = particles_on_edge;

    [y, x, z] = meshgrid(1:p_edge, 1:p_edge, 1:p_edge);

    xyz = cat(4, x, y, z);

    cubeposition = reshape(xyz, [numel(xyz)/3 3])';

    ax = 3;

    outputpos = cubeposition(:, cubeposition(ax, :) == 1);
    
    % Hacky hardcoding for the movement and relaxation time for now
    [ o, h ]=testlengths(strains, ax, 300, 300, [1 1 1], [p_edge p_edge p_edge], 10, particle_system,...
            outputpos, {'position', 'force'}, cubeposition, {'position'});
        
    positions = h{1};
    
    forces = gettotalaxisforce(o{2}, ax);
end