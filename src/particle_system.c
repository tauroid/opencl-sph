#include "particle_system.h"
#include "opencl/particle_system_host.h"
#ifndef MATLAB_MEX_FILE
    #include <stdlib.h>
#endif

static psdata * data = 0x0;

int exists_psdata_instance() {
    return data != 0x0;
}

psdata * get_psdata_instance() {
    if (data == 0x0) {
#ifndef MATLAB_MEX_FILE
        data = (psdata*) calloc(1, sizeof(psdata));
#else
        data = (psdata*) mxCalloc(1, sizeof(psdata));
        mexMakeMemoryPersistent(data);
#endif
    }

    return data;
}

void free_psdata_instance() {
#ifndef MATLAB_MEX_FILE
    free(data);
#else
    mxFree(data);
#endif

    data = 0x0;
}

void init_psdata( psdata* data, int pnum, double mass, double timestep, double smoothingradius ) {
    data->pnum = pnum;
    data->n = 0;
    data->mass = mass;
    data->timestep = timestep;
    data->smoothingradius = smoothingradius;

#ifndef MATLAB_MEX_FILE

    data->position     = (double*) calloc(POSITION_SIZE(data),     sizeof(double));
    data->posnext      = (double*) calloc(POSNEXT_SIZE(data),      sizeof(double));
    data->velocity     = (double*) calloc(VELOCITY_SIZE(data),     sizeof(double));
    data->veleval      = (double*) calloc(VELEVAL_SIZE(data),      sizeof(double));
    data->velnext      = (double*) calloc(VELNEXT_SIZE(data),      sizeof(double));
    data->acceleration = (double*) calloc(ACCELERATION_SIZE(data), sizeof(double));
    data->force        = (double*) calloc(FORCE_SIZE(data),        sizeof(double));
    data->density      = (double*) calloc(DENSITY_SIZE(data),      sizeof(double));
    data->volume       = (double*) calloc(VOLUME_SIZE(data),       sizeof(double));

#else

    int position_dims[]     = { POSITION_DIMS(data)     };
    int posnext_dims[]      = { POSNEXT_DIMS(data)      };
    int velocity_dims[]     = { VELOCITY_DIMS(data)     };
    int veleval_dims[]      = { VELEVAL_DIMS(data)      };
    int velnext_dims[]      = { VELNEXT_DIMS(data)      };
    int acceleration_dims[] = { ACCELERATION_DIMS(data) };
    int force_dims[]        = { FORCE_DIMS(data)        };
    int density_dims[]      = { DENSITY_DIMS(data)      };
    int volume_dims[]       = { VOLUME_DIMS(data)       };

    data->position_mex     = mxCreateNumericArray( sizeof(position_dims)/sizeof(int),
                                                   position_dims, mxDOUBLE_CLASS, mxREAL );
    data->posnext_mex      = mxCreateNumericArray( sizeof(posnext_dims)/sizeof(int),
                                                   posnext_dims, mxDOUBLE_CLASS, mxREAL );
    data->velocity_mex     = mxCreateNumericArray( sizeof(velocity_dims)/sizeof(int),
                                                   velocity_dims, mxDOUBLE_CLASS, mxREAL );
    data->veleval_mex      = mxCreateNumericArray( sizeof(veleval_dims)/sizeof(int),
                                                   veleval_dims, mxDOUBLE_CLASS, mxREAL );
    data->velnext_mex      = mxCreateNumericArray( sizeof(velnext_dims)/sizeof(int),
                                                   velnext_dims, mxDOUBLE_CLASS, mxREAL );
    data->acceleration_mex = mxCreateNumericArray( sizeof(acceleration_dims)/sizeof(int),
                                                   acceleration_dims, mxDOUBLE_CLASS, mxREAL );
    data->force_mex        = mxCreateNumericArray( sizeof(force_dims)/sizeof(int),
                                                   force_dims, mxDOUBLE_CLASS, mxREAL );
    data->density_mex      = mxCreateNumericArray( sizeof(density_dims)/sizeof(int),
                                                   density_dims, mxDOUBLE_CLASS, mxREAL );
    data->volume_mex       = mxCreateNumericArray( sizeof(volume_dims)/sizeof(int),
                                                   volume_dims, mxDOUBLE_CLASS, mxREAL );

    mexMakeArrayPersistent(data->position_mex);
    mexMakeArrayPersistent(data->posnext_mex);
    mexMakeArrayPersistent(data->velocity_mex);
    mexMakeArrayPersistent(data->veleval_mex);
    mexMakeArrayPersistent(data->velnext_mex);
    mexMakeArrayPersistent(data->acceleration_mex);
    mexMakeArrayPersistent(data->force_mex);
    mexMakeArrayPersistent(data->density_mex);
    mexMakeArrayPersistent(data->volume_mex);

    data->position     = mxGetPr(data->position_mex);
    data->posnext      = mxGetPr(data->posnext_mex);
    data->velocity     = mxGetPr(data->velocity_mex);
    data->veleval      = mxGetPr(data->veleval_mex);
    data->velnext      = mxGetPr(data->velnext_mex);
    data->acceleration = mxGetPr(data->acceleration_mex);
    data->force        = mxGetPr(data->force_mex);
    data->density      = mxGetPr(data->density_mex);
    data->volume       = mxGetPr(data->volume_mex);

#endif
}

void free_psdata( psdata * data ) {
#ifndef MATLAB_MEX_FILE

    free(data->position);
    free(data->posnext);
    free(data->velocity);
    free(data->veleval);
    free(data->velnext);
    free(data->acceleration);
    free(data->force);
    free(data->density);
    free(data->volume);

#else

    mxDestroyArray(data->position_mex);
    mxDestroyArray(data->posnext_mex);
    mxDestroyArray(data->velocity_mex);
    mxDestroyArray(data->veleval_mex);
    mxDestroyArray(data->velnext_mex);
    mxDestroyArray(data->acceleration_mex);
    mxDestroyArray(data->force_mex);
    mxDestroyArray(data->density_mex);
    mxDestroyArray(data->volume_mex);

#endif
}

void _increment_position_0_opencl( psdata * data ) {

}

void _decrement_position_0_opencl( psdata * data ) {

}
