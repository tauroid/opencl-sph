clear all;

num_steps = 2000;

testinit_wrap();

position = querystate('position_mex');
density = querystate('density_mex');
n = querystate('n_mex');

figure;
for i = 1:num_steps
    scatter3(position(1,1:n), position(2,1:n), position(3,1:n), 10, density(1:n));
    axis([0 3 0 3 0 3]);

    simstep();
    position = querystate('position_mex');
    density = querystate('density_mex');
    
    drawnow;
end