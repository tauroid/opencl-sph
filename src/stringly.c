#include <string.h>
#include <stdio.h>
#include "stringly.h"

// Matches FieldType
static const char * type_strings[] = {
    "char", 0x0,
    "unsigned int", "uint", 0x0,
    "int", 0x0,
    "float", "float32", 0x0,
    "double", "float64", 0x0
};

static const size_t type_sizes[] = {
    sizeof(char),
    sizeof(unsigned int),
    sizeof(int),
    4,
    8
};

void * allocate_string_typed_array(const char * type, size_t num_elements) {
    void * data = calloc(num_elements, sizeof_string_type(type));

    return data;
}

FieldType typeof_string_type(const char * type) {
    size_t count = 0;

    for (size_t i = 0; i < sizeof(type_strings)/sizeof(type_strings[0]); ++i) {
        if (type_strings[i] == 0x0) {
            ++count;
            continue;
        }

        if (strcmp(type_strings[i], type) == 0) {
            return count;
        }
    }

    return 0;
}

size_t sizeof_string_type(const char * type) {
    size_t count = 0;

    for (size_t i = 0; i < sizeof(type_strings)/sizeof(type_strings[0]); ++i) {
        if (type_strings[i] == 0x0) {
            ++count;
            continue;
        }

        if (strcmp(type_strings[i], type) == 0) {
            return type_sizes[count];
        }
    }

    return 0;
}
