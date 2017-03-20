function [ f_sum_axis ] = gettotalaxisforce( forces, axis )
%GETTOTALAXISFORCE Gets the sum of forces in axis direction

f_sum = sum(forces, 2);

f_sum_axis = squeeze(f_sum(axis, :, :));


end

