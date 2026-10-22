#include "../../src/core/table_control.hpp"
#include "../../src/sys/msg_print.h"
#include "../../src/tools/sha.h"
#include <cassert>
#include <cstdlib>
#include <cstring>




bool dir_tuples_cmp(dir_tuples_t * dt1, dir_tuples_t * dt2, int len);
int main()
{
    Dir_table_controller    dir_table_ctrl;
    dir_tuples_t *          dirs_expect = (dir_tuples_t *)calloc(1, sizeof(dir_tuples_t)+sizeof(dir_tuple_t));

    dirs_expect->size = 1;
    strncpy(dirs_expect->tuples[0].dirpath, "/Users/starless/Projects/Sync-Files-System/src/tests/send", sizeof(dirs_expect->tuples[0].dirpath));
    sha256(dirs_expect->tuples[0].dirpath_hash, dirs_expect->tuples[0].dirpath, strlen(dirs_expect->tuples[0].dirpath));
    dirs_expect->tuples[0].modify_time = 1791728992;
    dirs_expect->tuples[0].update_flag = 1;
    dirs_expect->tuples[0].deleted_flag = 0;

    dir_table_ctrl.bind("./test.db", "Sender_map_table");
    dir_table_ctrl.update_table(&dirs_expect->tuples[0]);

    dir_tuples_t * dirs_to_test = (dir_tuples_t *)dir_table_ctrl.get_manage_dirs();

    assert(dir_tuples_cmp(dirs_expect, dirs_to_test, sizeof(dir_tuples_t)+sizeof(dir_tuple_t)) == true);
    MSG_DBG_F("Dir_table_controller::get_manage_dirs() test passed.");

    dir_table_ctrl.delete_tuple(dirs_expect->tuples[0].dirpath);


}

bool dir_tuples_cmp(dir_tuples_t * dt1, dir_tuples_t * dt2, int len)
{
    if (dt1 == nullptr && dt2 != nullptr) {
        return false;
    }
    if (dt1 != nullptr && dt2 == nullptr) {
        return false;
    }
    if ( memcmp(dt1, dt2, len) == 0)
        return true;
    else
        return false;
}
