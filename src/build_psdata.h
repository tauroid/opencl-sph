#ifndef _BUILD_PSDATA_H_
#define _BUILD_PSDATA_H_

#include "particle_system.h"

void build_psdata_from_string(psdata * data, char * string); // Munges string
void build_psdata(psdata * data, const char * path);

#endif
