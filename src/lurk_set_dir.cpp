#include "common/check_error.hpp"
#include "core/table_control.hpp"
#include "config/get_config.hpp"
#include "utils/makedir.h"
#include "common/sha.h"
#include "common/msg_print.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// argv[1]: dirpath you want to set in database
int main(int argc, char * argv[])
{
try {
    if (argc < 2)
        throw std::runtime_error("parameter cnt is not match, current is: " + std::to_string(argc));

    char    dir_to_manage[4096];
    char    verify_path[4096];
    char    db_path[4096];
    memset(dir_to_manage, 0, sizeof(dir_to_manage));
    strncpy(dir_to_manage, argv[1], strlen(argv[1]));
    MSG_DBG_F("dir_to_manage is %s", argv[1]);

    if (strlen(dir_to_manage) > 4080) {
        printf("😓 Your dir '%s' is too long, cannot manage\n", dir_to_manage);
        return EXIT_FAILURE;
    }
    if (strlen(dir_to_manage) < 2) {
        printf("😓 Your dir '%s' is too small, cannot manage\n", dir_to_manage);
        return EXIT_FAILURE;
    }
    if ( make_dir(dir_to_manage) == false) {
        printf("🫥 Create '%s' failed, maybe priviliage problem\n", dir_to_manage);
        return EXIT_FAILURE;
    }

    // make sure the tools/makedir.h could work
    if (dir_to_manage[strlen(dir_to_manage)-1] != '/') {
        dir_to_manage[strlen(dir_to_manage)] = '/';
    }
    make_dir(dir_to_manage);
    dir_to_manage[strlen(dir_to_manage)-1] = 0;

    memset(verify_path, 0, sizeof(verify_path));
    get_config_verifypath(verify_path, sizeof(verify_path));
    make_dir(verify_path);
    memset(db_path, 0, sizeof(db_path));
    snprintf(db_path, sizeof(db_path), "%s/%s", verify_path, "lurk_database.db");

    Dir_table_controller dir_table_ctrl;
    if ( dir_table_ctrl.bind(db_path, "Sender_map_table") == false) {
        throw std::runtime_error("Open database failed, the path is: " + std::string(db_path));
    }

    struct stat file_stat;
    check_syscall_error(stat(argv[1], &file_stat), "stat");
    dir_tuple_t  dir_tuple;
    memset(&dir_tuple, 0, sizeof(dir_tuple));
    strncpy(dir_tuple.dirpath, dir_to_manage, strlen(dir_to_manage));
    sha256(dir_tuple.dirpath_hash, dir_to_manage, strlen(dir_to_manage));
    dir_tuple.modify_time = 0;
    dir_tuple.update_flag = 1;
    dir_tuple.deleted_flag = 0;
    if ( dir_table_ctrl.update_table(&dir_tuple) == false) {
        throw std::runtime_error("Update database failed, the path is: " + std::string(db_path));
    }
    printf("🫡 Manage dir is ok, update process will take care of your dir\n");
    return EXIT_SUCCESS;
} catch (std::runtime_error& e) {
    std::cerr << "[lurk-set-dir] failed : " << e.what() << std::endl;
} catch (std::exception& e) {
    std::cerr << "[lurk-set-dir] failed : " << e.what() << std::endl;
}
    return EXIT_FAILURE;
}
