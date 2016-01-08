#ifndef _PARTICLE_SYSTEM_H_
#define _PARTICLE_SYSTEM_H_

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

#define NUM_DIMENSIONS 3
/* TODO check and conform to matlab ordering scheme */
#define POSITION_DIMS(x)     NUM_DIMENSIONS, x->pnum
#define POSNEXT_DIMS(x)      NUM_DIMENSIONS, x->pnum
#define VELOCITY_DIMS(x)     NUM_DIMENSIONS, x->pnum
#define VELEVAL_DIMS(x)      NUM_DIMENSIONS, x->pnum
#define VELNEXT_DIMS(x)      NUM_DIMENSIONS, x->pnum
#define ACCELERATION_DIMS(x) NUM_DIMENSIONS, x->pnum
#define FORCE_DIMS(x)        NUM_DIMENSIONS, x->pnum
#define DENSITY_DIMS(x)      x->pnum
#define VOLUME_DIMS(x)       x->pnum

#define POSITION_SIZE(x)     NUM_DIMENSIONS * x->pnum
#define POSNEXT_SIZE(x)      NUM_DIMENSIONS * x->pnum
#define VELOCITY_SIZE(x)     NUM_DIMENSIONS * x->pnum
#define VELEVAL_SIZE(x)      NUM_DIMENSIONS * x->pnum
#define VELNEXT_SIZE(x)      NUM_DIMENSIONS * x->pnum
#define ACCELERATION_SIZE(x) NUM_DIMENSIONS * x->pnum
#define FORCE_SIZE(x)        NUM_DIMENSIONS * x->pnum
#define DENSITY_SIZE(x)      x->pnum
#define VOLUME_SIZE(x)       x->pnum

typedef struct
{
    int pnum;
    int n;
    double mass;
    double timestep;
    double smoothingradius;

    double * position;
    double * posnext;
    double * velocity;
    double * veleval;
    double * velnext;
    double * acceleration;
    double * force;
    double * density;
    double * volume;

#ifdef MATLAB_MEX_FILE
    mxArray * position_mex;
    mxArray * posnext_mex;
    mxArray * velocity_mex;
    mxArray * veleval_mex;
    mxArray * velnext_mex;
    mxArray * acceleration_mex;
    mxArray * force_mex;
    mxArray * density_mex;
    mxArray * volume_mex;
#endif
} psdata;

int exists_psdata_instance();
psdata * get_psdata_instance();
void free_psdata_instance();
void init_psdata( psdata * data, int pnum, double mass, double timestep, double smoothingradius );
void free_psdata( psdata * data );

void _increment_position_0_opencl( psdata * data );
void _decrement_position_0_opencl( psdata * data );

#endif
