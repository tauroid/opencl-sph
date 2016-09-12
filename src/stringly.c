#include <stdlib.h>
#include <string.h>
#include "stringly.h"

void * allocate_string_typed_array(const char * type, size_t num_elements) {
    void * data = calloc(num_elements, sizeof_string_type(type));

    return data;
}

size_t sizeof_string_type(const char * type) {
    if (strcmp("float", type) == 0) {
        return sizeof(float);
    } else if (strcmp("unsigned int", type) == 0) {
        return sizeof(unsigned int);
    } else if (strcmp("double", type) == 0) {
        return sizeof(double);
    } else if (strcmp("int", type) == 0) {
        return sizeof(int);
    }

    return 0;
}
