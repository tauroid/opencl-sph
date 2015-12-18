#ifndef _SOLID_H_
#define _SOLID_H_

#include "particle_system.h"

class Solid : public ParticleSystem
{
    Solid ( int pnum, double mass, double timestep, double smoothingradius ) :
        ParticleSystem(pnum, mass, timestep, smoothingradius)
    {

    }
}


#endif
