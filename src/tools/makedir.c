#include "makedir.h"
#include "tools/msg_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

bool make_dir(const char *path)
{
    if ( path == NULL)
        return false;
    char    path_tmp[strlen(path)+100];

    memset(path_tmp, 0, sizeof(path_tmp));
    if (path[0] == '~') {
        const char * home = getenv("HOME");
        snprintf(path_tmp, sizeof(path_tmp), "%s/%s", home, path+2);
    } else {
        strncpy(path_tmp, path, strlen(path)+100);
    }

    MSG_DBG_F("Will Create path %s", path_tmp);

    int size = strlen(path_tmp);
    for (int i = 0; i < size; i++) {
        // skip "./"
        if (i < size-1 && path_tmp[i] == '.' && path_tmp[i+1] == '/') {
            i++;
            continue;
        }

        // skip "../"
        if (i < size-2 && path_tmp[i] == '.' && path_tmp[i+1] == '.' && path_tmp[i+2] == '/') {
            i += 2;
            continue;
        }

        if (i != 0 && path_tmp[i] == '/') {
            path_tmp[i] = 0;
            // MSG_DBG_F("Create path %s", path_tmp);
            if (access(path_tmp, F_OK) == -1) {
                if ( mkdir(path_tmp, 0755) == -1) {
                    MSG_DBG_F("mkdir failed %s : %s", path_tmp, strerror(errno));
                    return false;
                }
            }
            path_tmp[i] = '/';
        }
    }
    return true;
}
