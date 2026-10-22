#include "core/table_control.hpp"
#include "tools/get_config.h"
#include "tools/makedir.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>


int main()
{
    Dir_table_controller dir_table_ctrl;

    char verify_path[4096];
    char db_path[4096];

    memset(verify_path, 0, sizeof(verify_path));
    if (get_config_verifypath_client(verify_path) == false) {
        printf("get verifypath failed\n");
        return EXIT_FAILURE;
    }

    memset(db_path, 0, sizeof(db_path));
    snprintf(db_path, sizeof(db_path), "%s/%s", verify_path, "lurk_database.db");
    make_dir(db_path);

    dir_table_ctrl.bind(db_path, "Sender_map_table");

    dir_tuples_t * dir_tuples;
    if ( (dir_tuples = (dir_tuples_t *)dir_table_ctrl.get_tuples(10, 0))) {
        for (int i = 0; i < dir_tuples->size; i++) {
            // if (dir_tuples->tuples[i].update_flag == 1) {
            //     printf("Updated: %s, Dir: '%s'\n", "Yes", dir_tuples->tuples[i].dirpath);
            // } else {
            //     printf("Updated: %s, Dir: '%s'\n", "No", dir_tuples->tuples[i].dirpath);
            // }

            printf("Dir: '%s', update_flag: %hu, deleted_flag: %hu\n", dir_tuples->tuples[i].dirpath, dir_tuples->tuples[i].update_flag, dir_tuples->tuples[i].deleted_flag);
        }
    } else {
        printf("You have not set any dir to lurk\n");
    }

    // dir_tuples_t * dir_tuples;
    // if ( (dir_tuples = (dir_tuples_t *)dir_table_ctrl.get_manage_dirs()) != nullptr) {
    //     for (int i = 0; i < dir_tuples->size; i++) {
    //         if (dir_tuples->tuples[i].update_flag == 1) {
    //             printf("Updated: %s, Dir: '%s'\n", "Yes", dir_tuples->tuples[i].dirpath);
    //         } else {
    //             printf("Updated: %s, Dir: '%s'\n", "No", dir_tuples->tuples[i].dirpath);
    //         }
    //     }
    // } else {
    //     printf("You have not set any dir to lurk\n");
    // }



}
