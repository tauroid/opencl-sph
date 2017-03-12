#include <math.h>
#include <string.h>
#ifndef MATLAB_MEX_FILE
    #include <stdlib.h>
#else
    #include "mex.h"
#endif
#include "note.h"
#include "particle_system.h"
#include "build_psdata.h"
#include "opencl/particle_system_host.h"

static psdata * ps_instance = NULL;

#ifdef MATLAB_MEX_FILE
psdata * create_stored_psdata_from_string(const char * string) {
    ps_instance = malloc(sizeof(psdata));

    ps_instance->num_fields = 0;

    build_psdata_from_string(ps_instance, string);

    return ps_instance;
}

psdata * get_stored_psdata() {
    return ps_instance;
}

void free_stored_psdata() {
    free_psdata(ps_instance);

    free(ps_instance);
}
#endif

// These need severe looking at because of the float thing
void display_entry(psdata data, size_t offset, size_t size) {
    if (size == 8) {
        note(2, "%g, ", *((double*)((char*)data.data + offset)));
    } else if (size == 4) {
        note(2, "%u, ", *((unsigned int*)((char*)data.data + offset)));
    }
}

// And you
void display_psdata(psdata data, const char * const * mask) {
    if (mask == NULL) {
        for (size_t field = 0; field < data.num_fields; ++field) {
            note(2, "%s:\n\n", data.names + data.names_offsets[field]);
            if (data.num_dimensions[field] == 1) {
                unsigned int d0 = (data.dimensions + data.dimensions_offsets[field])[0];
                for (size_t i = 0; i < d0; ++i) {
                    display_entry(data, data.data_offsets[field] + i*data.entry_sizes[field], data.entry_sizes[field]);
                }
            } else if (data.num_dimensions[field] == 2) {
                unsigned int d0 = (data.dimensions + data.dimensions_offsets[field])[0];
                unsigned int d1 = (data.dimensions + data.dimensions_offsets[field])[1];
                for (unsigned int i = 0; i < d1; ++i) {
                    for (unsigned int j = 0; j < d0; ++j) {
                        display_entry(data, data.data_offsets[field] + (i*d0+j)*data.entry_sizes[field], data.entry_sizes[field]);
                    }
                    note(2, "\n");
                }
            } else if (data.num_dimensions[field] == 3) {
                unsigned int d0 = (data.dimensions + data.dimensions_offsets[field])[0];
                unsigned int d1 = (data.dimensions + data.dimensions_offsets[field])[1];
                unsigned int d2 = (data.dimensions + data.dimensions_offsets[field])[2];
                for (unsigned int i = 0; i < d2; ++i) {
                    for (unsigned int j = 0; j < d1; ++j) {
                        for (unsigned int k = 0; k < d0; ++k) {
                            display_entry(data, data.data_offsets[field] + (i*d1*d0+j*d0+k)*data.entry_sizes[field], data.entry_sizes[field]);
                        }
                        note(2, "\n");
                    }
                    note(2, "\n");
                }
            }
            note(2, "\n\n");
        }
    }
}

int get_field_psdata(psdata data, const char * name) {
    unsigned int i;
    for (i = 0; i < data.num_fields; ++i) {
        if (strcmp(data.names + data.names_offsets[i], name) == 0) return i;
    }

    return -1;
}

void set_field_psdata(psdata * data, const char * name, void * field, unsigned int size, unsigned int offset) {
    int i = get_field_psdata(*data, name);

    if (i != -1) memcpy((char*)data->data + data->data_offsets[i] + offset, field, size);
}

unsigned int psdata_names_size(psdata data) {
    char * name = (char*) data.names + data.names_offsets[data.num_fields-1];
    return (data.names_offsets[data.num_fields-1]
          + strlen(name) + 1) * sizeof(char);
}

unsigned int psdata_dimensions_size(psdata data) {
    unsigned int nf = data.num_fields;
    return (data.dimensions_offsets[nf-1] + data.num_dimensions[nf-1]) * sizeof(unsigned int);
}

unsigned int psdata_data_size(psdata data) {
    return data.data_offsets[data.num_fields-1] + data.data_sizes[data.num_fields-1];
}

/* Data needs to be heap allocated but it should be anyway, right? RIGHT? */
/* Don't worry I'll allocate that string for you.. */
int create_host_field_psdata(psdata * data, const char * name, void * field, unsigned int size) {
    unsigned int new_num_host_fields = data->num_host_fields + 1;

    /* Allocate */
    char ** new_host_names = malloc(new_num_host_fields*sizeof(char*));
    void ** new_host_data = malloc(new_num_host_fields*sizeof(void*));
    unsigned int * new_host_data_size = malloc(new_num_host_fields*sizeof(unsigned int));

    /* Copy */
    unsigned int i;
    for (i = 0; i < data->num_host_fields; ++i) {
        new_host_names[i] = data->host_names[i];
        new_host_data[i] = data->host_data[i];
        new_host_data_size[i] = data->host_data_size[i];
    }

    /* Free old stuff */
    if (data->num_host_fields > 0) {
        free(data->host_names);
        free(data->host_data);
        free(data->host_data_size);
    }

    char * name_alloc = malloc((strlen(name)+1)*sizeof(char));
    strcpy(name_alloc, name);

    /* Add new field */
    new_host_names[i] = name_alloc;
    new_host_data[i] = field;
    new_host_data_size[i] = size;

    data->num_host_fields = new_num_host_fields;

    /* Attach to struct */
    data->host_names = new_host_names;
    data->host_data = new_host_data;
    data->host_data_size = new_host_data_size;

    return i;
}

int get_host_field_psdata(psdata * data, const char * name) {
    unsigned int i;
    for (i = 0; i < data->num_host_fields; ++i) {
        if (strcmp(data->host_names[i], name) == 0) return i;
    }

    return -1;
}

#ifdef MATLAB_MEX_FILE
void sync_to_mex(psdata * data) {
    void * field;
    mxArray * mex_field;

    int i, f;
    for (i = 0; i < data->num_host_fields; ++i) {
        if (!is_mex_field(data->host_names[i])) continue;

        mex_field = (mxArray*) data->host_data[i];

        int name_length = strlen(data->host_names[i]) - 4;
        char * field_name = malloc((name_length + 1)*sizeof(char));
        strncpy(field_name, data->host_names[i], name_length);
        field_name[name_length] = '\0';
        f = get_field_psdata(*data, field_name);

        free(field_name);

        if (f == -1) continue;

        void * mex_field_ptr = mxGetData(mex_field);
        
        memcpy(mex_field_ptr, ((char*) data->data) + data->data_offsets[f], data->data_sizes[f]);
    }
}

int is_mex_field(const char * name) {
    const char * mex_cmp = "_mex";
    size_t mex_len = strlen(mex_cmp);

    size_t name_len = strlen(name);
    if (name_len < mex_len) return 0;

    const char * name_end = name + name_len - mex_len;
    if (strcmp(name_end, mex_cmp) == 0) return 1;
    
    return 0;
}

void get_mex_field_name(const char * name, size_t * pReturnLength, char * mex_name) {
    const char * mext = "_mex";

    if (pReturnLength != NULL) {
        *pReturnLength = strlen(name) + strlen(mext);
        return;
    } else {
        sprintf(mex_name, "%s%s", name, mext);
    }
}
#endif

void free_psdata( psdata * data ) {
    free((char*) data->names);
    free(data->names_offsets);
    
    free(data->dimensions);
    free(data->num_dimensions);
    free(data->dimensions_offsets);
    free(data->entry_sizes);

#ifdef MATLAB_MEX_FILE
    mxFree(data->data);
#else
    free(data->data);
#endif
    free(data->data_sizes);
    free(data->data_offsets);

    free(data->data_ptrs);

    unsigned int i;
    for (i = 0; i < data->num_host_fields; ++i) {
#ifdef MATLAB_MEX_FILE
        if (is_mex_field(data->host_names[i])) mxDestroyArray(data->host_data[i]);
        else free(data->host_data[i]);
#else
        free(data->host_data[i]);
#endif

        free(data->host_names[i]);
    }
    note(1, "freed host data\n");

    if (data->num_host_fields > 0) {
        free(data->host_names);
        free(data->host_data);
        free(data->host_data_size);
    }
}
