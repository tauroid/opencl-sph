#ifndef DATATYPES_H_
#define DATATYPES_H_

#include <stdint.h>

typedef enum {
    T_CHAR,
    T_UINT,
    T_INT,
    T_FLOAT32,
    T_FLOAT64,
    TYPE_COUNT
} FieldType;

typedef struct {
    FieldType type;
    union {
        char * c;
        uint32_t * u;
        int32_t * i;
        float * f4;
        double * f8;
        void * ptr;
    };
} DataPtr;

#endif
