#include "config/get_config.hpp"
#include "common/msg_print.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string.h>

// argv[1] : recv_dir for server
int main(int argc, char * argv[])
{
try {
    if (argc != 2) {
        throw std::runtime_error("parameter cnt is not match, current is: " + std::to_string(argc));
    }
    char recv_path[4096];
    memset(recv_path, 0, sizeof(recv_path));
    strncpy(recv_path, argv[1], strlen(argv[1]));
    init_config_server(recv_path);

    return EXIT_SUCCESS;
} catch (std::runtime_error& e) {
    std::cerr << "[lurk-server-init] failed : " << e.what() << std::endl;
} catch (std::exception& e) {
    std::cerr << "[lurk-server-init] failed : " << e.what() << std::endl;
}
    return EXIT_FAILURE;
}
