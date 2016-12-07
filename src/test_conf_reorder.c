#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "build_psdata.h"
#include "3rdparty/whereami.h"

void build_psdata_from_config(psdata * pData, const char * relativePath)
{
    int exe_path_len = wai_getExecutablePath(NULL, 0, NULL);
    char * exe_path = malloc((exe_path_len+1)*sizeof(char));
    wai_getExecutablePath(exe_path, exe_path_len, NULL);

    exe_path[exe_path_len] = 0x0;
    char * lastslash = strrchr(exe_path, '/');

    if (lastslash != NULL) {
        exe_path_len = (int)( (lastslash - exe_path) / sizeof(char) );
        exe_path[exe_path_len] = 0x0;
    }

    char * conf_path = malloc((strlen(exe_path)+strlen(relativePath)+1)*sizeof(char));
    sprintf(conf_path, "%s%s", exe_path, relativePath);

    build_psdata(pData, conf_path);

    free(exe_path); free(conf_path);
}

int main(void) {
    psdata data;

    build_psdata_from_config(&data, "/../conf/solid.conf");

    free_psdata(&data);

    return 0;
}
