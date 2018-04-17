#ifndef BUILD_PSDATA_H_
#define BUILD_PSDATA_H_

#include "particle_system.h"

void build_psdata_from_string(psdata * data, const char * string);
void build_psdata(psdata * data, const char * path);

typedef struct psdata_field_spec psdata_field_spec;

struct psdata_field_spec
{
    char * name;
    unsigned int num_dimensions;
    unsigned int * dimensions;
    char * type; // Only double and unsigned int for now
    void * data; /* Optional */

    psdata_field_spec * next;
};

#endif
