#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "3rdparty/whereami.h"
#include "note.h"
#ifdef MATLAB_MEX_FILE
    #include "mex.h"
#endif

#define MAX_SECTIONS 8

static char * g_config = NULL;
static size_t g_num_sections = 0;
static char * g_section_titles[MAX_SECTIONS] = {NULL};
static char * g_section_starts[MAX_SECTIONS] = {NULL};

int load_config(const char * relativePath) {
    /* Let's stick to relative for now
#ifndef MATLAB_MEX_FILE
    // Get the absolute path of the config
    int exe_path_len = wai_getExecutablePath(NULL, 0, NULL);
    char * exe_path = malloc((exe_path_len+1)*sizeof(char));
    wai_getExecutablePath(exe_path, exe_path_len, NULL);

    exe_path[exe_path_len] = 0x0;
    char * lastslash = strrchr(exe_path, '/');

    if (lastslash != NULL) {
        exe_path_len = (int)( (lastslash - exe_path) / sizeof(char) );
        exe_path[exe_path_len] = '\0';
    }
#else
    char * exe_path = getenv("EXE_PATH");
#endif

    char * conf_path = malloc((strlen(exe_path)+strlen(relativePath)+1)*sizeof(char));
    sprintf(conf_path, "%s%s", exe_path, relativePath);

#ifndef MATLAB_MEX_FILE
    free(exe_path);
#endif
    */

    // Copy the contents
    FILE * conf = fopen(relativePath, "rb");

    if (conf == NULL) {
        note(2, "Could not read file %s\n", relativePath);
        return 1;
    }

    // free(conf_path);

    fseek(conf, 0, SEEK_END);
    size_t conf_end = ftell(conf);

    g_config = malloc(conf_end+1*sizeof(char));

    fseek(conf, 0, SEEK_SET);
    fread(g_config, 1, conf_end, conf);
    fclose(conf);

    g_config[conf_end] = '\0';

    // Extract position of section headings and contents and chop the string up so they are isolated
    char * section_pointer = g_config;

    if (conf_end > 0 && g_config[0] == '$') {
        section_pointer = g_config + 1;
        strtok(section_pointer, "\n");
    } else {
        strtok(section_pointer, "$");
        section_pointer = strtok(NULL, "\n");
    }

    while (section_pointer != NULL &&
           g_num_sections < MAX_SECTIONS) {
        g_section_titles[g_num_sections] = section_pointer;

        section_pointer = strtok(NULL, "$");

        g_section_starts[g_num_sections] = section_pointer;
        if (section_pointer != NULL) ++g_num_sections;

        section_pointer = strtok(NULL, "\n");
    }

    return 0;
}
const char * get_config_section(const char * heading) {
    for (size_t i = 0; i < g_num_sections; ++i) {
        if (strcmp(heading, g_section_titles[i]) == 0) return g_section_starts[i];
    }

    return NULL;
}
void unload_config() {
    free(g_config);

    g_config = NULL;
    g_num_sections = 0;
    memset(g_section_starts, 0, sizeof g_section_starts);
    memset(g_section_titles, 0, sizeof g_section_titles);
}
