ss = StrainSolid;

strains = -0.5:0.05:0.5;

edge_numbers = 11:12;

forces = cell(numel(edge_numbers), 1);
position_history = cell(numel(edge_numbers), 1);

for i = 1:numel(edge_numbers)
    ss.reset();

    [f, ph] = runtestlengths(edge_numbers(i), strains, ss);
    
    forces{i} = f;
    position_history{i} = ph;
end

%predicted_forces = getpredictedforce(strains, 1, querystate('poisson'), querystate('youngs_modulus'));
predicted_forces = strains*querystate('youngs_modulus');

plot(strains, predicted_forces);

hold all;

legend_strings = cell(numel(edge_numbers), 1);

for i = 1:numel(edge_numbers)
    plot(strains, forces{i});
    
    legend_strings{i} = sprintf('%d^3 particles', edge_numbers(i));
end

hold off;

legend('Predicted output', legend_strings{:}, 'Location', 'southeast');

xlabel('Imposed strain');
ylabel('Total Z force on plane (z(\epsilon = 0) = -0.5)');
