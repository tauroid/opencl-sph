clear all;

num_steps = 2000;

testinit_fluid_wrap();

[position, density, n] = querystate('position_mex', 'density_mex', 'n_mex');

figure;
for i = 1:num_steps
    scatter3(position(1,1:n), position(2,1:n), position(3,1:n), 10, density(1:n));
    axis([-1.5 1.5 -1.5 1.5 -1.5 1.5]);

    simstep_fluid();
    [position, density] = querystate('position_mex', 'density_mex');
    
    drawnow;
end
