classdef StrainSolid < handle
    %STRAINSOLID Wrapper for the corotated linear solid implementation
    
    properties
        conf_path = '/../../conf/strainsolid.conf';
        initposition;
    end
    
    methods
        function init(obj, position, particlemass)
            obj.initposition = position;
            
            init_wrap(obj.conf_path);
            
            setstate('position', position, 'n', uint32(size(position, 2)));
            setstate('mass', particlemass);
            
            callkernel('init_original_position');
            
            computebins();
            
            callkernel('compute_original_density');
        end
        
        function step(obj)
            computebins();
            
            callkernel('compute_density');
            callkernel('compute_rotations_and_strains');
            callkernel('compute_stresses');
            callkernel('compute_forces_solids');
            
            callkernel('step_forward');
        end
        
        function reset(obj)
            clear initsph callkernel computebins querystate setstate simstep syncalldevicetohost syncallhosttodevice;
        end
    end
    
end

