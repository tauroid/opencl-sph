#ifndef STRINGLY_H_
#define STRINGLY_H_

#include <stdlib.h>

#include "datatypes.h"

void * allocate_string_typed_array(const char * type, size_t num_elements);
FieldType typeof_string_type(const char * type);
size_t sizeof_string_type(const char * type);

#endif
