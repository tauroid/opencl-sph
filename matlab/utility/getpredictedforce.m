function [ prediction ] = getpredictedforce( strains, originalside, poisson, youngs_modulus )
%GETPREDICTEDFORCE Get predicted force curve assuming linear deformation
%   Assuming a constant poisson's ratio, the interface area grows
%   proportionally to the square of the strain. This is multiplied by the
%   stress (youngs modulus times strain)

prediction = strains.*(originalside^2 * youngs_modulus).*(1 - poisson*strains).^2;
end

