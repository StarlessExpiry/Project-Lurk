#include "tools/get_config.h"
#include "tools/msg_print.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>

// argv[1] : recv_dir for server
int main(int argc, char * argv[])
{
    if (argc != 2) {
        MSG_DBG_F("Parameter cnt mismatch");
        return EXIT_FAILURE;
    }
    char recv_path[4096];
    memset(recv_path, 0, sizeof(recv_path));
    strncpy(recv_path, argv[1], strlen(argv[1]));
    MSG_DBG_F("recv_path: %s", recv_path);
    if ( init_config_server(recv_path) == false) {
        printf("Init server config file failed\n");
        return EXIT_FAILURE;
    } else {
        // printf("Init server config file success\n");
        return EXIT_SUCCESS;
    }
}
