#ifndef _PARTICLE_SYSTEM_H_
#define _PARTICLE_SYSTEM_H_

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

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
void do_opencl_whatever();

#endif
