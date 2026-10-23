#include "table_control.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <string>
extern "C" {
    #include "tools/makedir.h"
    #include "tools/msg_print.h"
    #include <fts.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <time.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/stat.h>
    #include <string.h>
    #include <fcntl.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <dirent.h>
    #include <sys/types.h>
    #include <fts.h>
}


sqlite3 * DB_controller::_db = nullptr;


bool DB_controller::bind(const char * database_path, const char * table_name) {
    // const char *    sql_sentence_str;
    // char            sql_sentence[5096];
    // char *          sql_error_msg = nullptr;
    make_dir(database_path);

    if ( _db == nullptr) {
        if( sqlite3_open(database_path, &_db) != 0) { // if it is not exist, create it
           MSG_DBG_F("Can't open database '%s'", database_path);
           goto failed;
        }
    }


    MSG_DBG("Database init successfully");
    // if ( sql_error_msg != nullptr)
    //     sqlite3_free(sql_error_msg);
    return true;
failed:
    // if ( sql_error_msg != nullptr)
    //     sqlite3_free(sql_error_msg);
    return false;
}

void DB_controller::free_tuples(void *tuples) {
    if (tuples != nullptr)
        free(tuples);
}

bool Dir_table_controller::bind(const char * database_path, const char * table_name) {
    const char *    sql_sentence_str;
    char            sql_sentence[5096];
    char *          sql_error_msg = nullptr;

    if ( make_dir(database_path) == false) {
        MSG_DBG_F("path create failed");
        return false;
    } else {
        MSG_DBG_F("path create success");
    }
    // DB_controller only bind database, and Dir_table_controller only bind table_name
    if ( DB_controller::bind(database_path, nullptr) == false) {
        MSG_DBG_F("Database bind failed");
        return false;

    } else {
        MSG_DBG_F("Database bind success");
    }

    sql_sentence_str =  "CREATE TABLE IF NOT EXISTS %s (\n"\
                        "    dirpath       TEXT    UNIQUE,\n"\
                        "    dirpath_hash  TEXT    UNIQUE,\n"\
                        "    modify_time   BIGINT  NOT NULL,\n" \
                        "    update_flag   INT     NOT NULL,\n" \
                        "    deleted_flag  INT     NOT NULL,\n" \
                        "    PRIMARY KEY(dirpath, dirpath_hash)\n"\
                        ");";
    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, table_name);
    if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
        MSG_DBG_F("Create table %s error: %s", table_name, sql_error_msg);
        goto failed;
    }
    strncpy(_db_table_name, table_name, sizeof(_db_table_name));

    if ( sql_error_msg != nullptr)
        sqlite3_free(sql_error_msg);
    return true;
failed:
    if ( sql_error_msg != nullptr)
        sqlite3_free(sql_error_msg);
    return false;
}

void * Dir_table_controller::get_tuples(int limit, int offset) {
     const char *            sql_sentence_str;
     char                    sql_sentence[5096];
     sqlite3_stmt *          sql_stmt = nullptr;
     dir_tuples_t *           table_tuples = nullptr;
     std::queue<dir_tuple_t*> dir_qu;
     int                     insert_pos = 0;

     sql_sentence_str =  "SELECT dirpath, dirpath_hash, modify_time, update_flag, deleted_flag\n" \
                         "FROM %s\n"\
                         "LIMIT %d OFFSET %d;";

     memset(sql_sentence, 0, sizeof(sql_sentence));
     snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, limit, offset);

     if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
         MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
         goto failed;
     }
     while ( (sqlite3_step(sql_stmt)) == SQLITE_ROW) {
         dir_tuple_t * dir_tuple_p = (dir_tuple_t *)calloc(1, sizeof(dir_tuple_t));
         strncpy(dir_tuple_p->dirpath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(dir_tuple_p->dirpath));
         strncpy(dir_tuple_p->dirpath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(dir_tuple_p->dirpath_hash));
         dir_tuple_p->modify_time = sqlite3_column_int64(sql_stmt, 2);
         dir_tuple_p->update_flag = sqlite3_column_int(sql_stmt, 3);
         dir_tuple_p->deleted_flag = sqlite3_column_int(sql_stmt, 4);

         dir_qu.push(dir_tuple_p);
     }


     if ( dir_qu.size() == 0) {
         table_tuples = nullptr;
         goto success;
     } else {
         table_tuples = (dir_tuples_t *)calloc(1, sizeof(dir_tuples_t) + dir_qu.size() * sizeof(dir_tuple_t));
     }
     table_tuples->size = dir_qu.size();

     while (!dir_qu.empty()) {
         memcpy((char *)&table_tuples->tuples[insert_pos], (const char *)dir_qu.front(), sizeof(dir_tuple_t));
         insert_pos++;
         free(dir_qu.front());
         dir_qu.pop();
     }

 success:
     if (sql_stmt != nullptr) {
         sqlite3_finalize(sql_stmt);
         sql_stmt = nullptr;
     }
     return table_tuples;
 failed:
     if (sql_stmt != nullptr) {
         sqlite3_finalize(sql_stmt);
         sql_stmt = nullptr;
     }
     if (table_tuples != nullptr) free(table_tuples);
     return nullptr;
}


// void Dir_table_controller::free_tuples(void * tuples) {
//     if (tuples != nullptr)
//         free(tuples);
// }
void * Dir_table_controller::get_update_tuples(int limit, int offset) {
     const char *            sql_sentence_str;
     char                    sql_sentence[5096];
     sqlite3_stmt *          sql_stmt = nullptr;
     dir_tuples_t *           table_tuples = nullptr;
     std::queue<dir_tuple_t*> dir_qu;
     int                     insert_pos = 0;

     sql_sentence_str =  "SELECT dirpath, dirpath_hash, modify_time, update_flag, deleted_flag\n" \
                         "FROM %s\n"\
                         "WHERE update_flag = 1\n"\
                         "LIMIT %d OFFSET %d;";

     memset(sql_sentence, 0, sizeof(sql_sentence));
     snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, limit, offset);

     if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
         MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
         goto failed;
     }
     while ( (sqlite3_step(sql_stmt)) == SQLITE_ROW) {
         dir_tuple_t * dir_tuple_p = (dir_tuple_t *)calloc(1, sizeof(dir_tuple_t));
         strncpy(dir_tuple_p->dirpath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(dir_tuple_p->dirpath));
         strncpy(dir_tuple_p->dirpath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(dir_tuple_p->dirpath_hash));
         dir_tuple_p->modify_time = sqlite3_column_int64(sql_stmt, 2);
         dir_tuple_p->update_flag = sqlite3_column_int(sql_stmt, 3);
         dir_tuple_p->deleted_flag = sqlite3_column_int(sql_stmt, 4);

         dir_qu.push(dir_tuple_p);
     }


     if ( dir_qu.size() == 0) {
         table_tuples = nullptr;
         goto success;
     } else {
         table_tuples = (dir_tuples_t *)calloc(1, sizeof(dir_tuples_t) + dir_qu.size() * sizeof(dir_tuple_t));
     }
     table_tuples->size = dir_qu.size();

     while (!dir_qu.empty()) {
         memcpy((char *)&table_tuples->tuples[insert_pos], (const char *)dir_qu.front(), sizeof(dir_tuple_t));
         insert_pos++;
         free(dir_qu.front());
         dir_qu.pop();
     }

 success:

     if (sql_stmt != nullptr) {
         sqlite3_finalize(sql_stmt);
         sql_stmt = nullptr;
     }
     return table_tuples;
 failed:
     if (sql_stmt != nullptr) {
         sqlite3_finalize(sql_stmt);
         sql_stmt = nullptr;
     }
     if (table_tuples != nullptr) free(table_tuples);
     return nullptr;
}



void * Dir_table_controller::get_deleted_tuples(int limit, int offset) {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    dir_tuples_t *           table_tuples = nullptr;
    std::queue<dir_tuple_t*> dir_qu;
    int                     insert_pos = 0;

    sql_sentence_str =  "SELECT dirpath, dirpath_hash, modify_time, update_flag, deleted_flag\n" \
                        "FROM %s\n"\
                        "WHERE deleted_flag = 1\n"\
                        "LIMIT %d OFFSET %d;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, limit, offset);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    while ( (sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        dir_tuple_t * dir_tuple_p = (dir_tuple_t *)calloc(1, sizeof(dir_tuple_t));
        strncpy(dir_tuple_p->dirpath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(dir_tuple_p->dirpath));
        strncpy(dir_tuple_p->dirpath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(dir_tuple_p->dirpath_hash));
        dir_tuple_p->modify_time = sqlite3_column_int64(sql_stmt, 2);
        dir_tuple_p->update_flag = sqlite3_column_int(sql_stmt, 3);
        dir_tuple_p->deleted_flag = sqlite3_column_int(sql_stmt, 4);

        dir_qu.push(dir_tuple_p);
    }


    if ( dir_qu.size() == 0) {
        table_tuples = nullptr;
        goto success;
    } else {
        table_tuples = (dir_tuples_t *)calloc(1, sizeof(dir_tuples_t) + dir_qu.size() * sizeof(dir_tuple_t));
    }
    table_tuples->size = dir_qu.size();

    while (!dir_qu.empty()) {
        memcpy((char *)&table_tuples->tuples[insert_pos], (const char *)dir_qu.front(), sizeof(dir_tuple_t));
        insert_pos++;
        free(dir_qu.front());
        dir_qu.pop();
    }

success:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return table_tuples;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    if (table_tuples != nullptr) free(table_tuples);
    return nullptr;
}


bool Dir_table_controller::update_table(void * tuple) {
//     if (tuple == nullptr) {
//         MSG_DBG("Parameter error");
//         return false;
//     }
//     const char *    sql_sentence_str;
//     char            sql_sentence[5096];
//     char *          sql_error_msg = nullptr;
//     dir_tuple_t *   dir_tuple;
//
//     dir_tuple = (dir_tuple_t *)tuple;
//
//     sql_sentence_str =  "INSERT INTO %s (dirpath, dirpath_hash, modify_time, update_flag, deleted_flag)\n" \
//                         "VALUES ('%s', '%s', %ld, %d, %d);";
//
//     memset(sql_sentence, 0, sizeof(sql_sentence));
//     snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, dir_tuple->dirpath, dir_tuple->dirpath_hash, dir_tuple->modify_time, dir_tuple->update_flag, dir_tuple->deleted_flag);
//
//     // if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence_str), &sql_stmt, NULL) != SQLITE_OK) {
//     //     MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
//     //     return false;
//     // }
//
//     // As insert failed, it means there has already a same primary key in table, so update the tuple is enough
//     if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
//         sql_sentence_str =  "UPDATE %s\n" \
//                             "SET modify_time = %ld\n" \
//                             "WHERE dirpath = '%s'";
//
//         memset(sql_sentence, 0, sizeof(sql_sentence));
//         snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, dir_tuple->modify_time, dir_tuple->dirpath);
//
//         if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
//             MSG_DBG_F("Insert a metadata of file error: %s", sql_error_msg);
//             goto failed;
//         }
//     }
//
//     if ( sql_error_msg != nullptr)
//         sqlite3_free(sql_error_msg);
//     return true;
// failed:
//     if ( sql_error_msg != nullptr)
//         sqlite3_free(sql_error_msg);
//     return false;

    dir_tuple_t *   dir_tuple = (dir_tuple_t *)tuple;
    sqlite3_stmt *  sql_stmt = nullptr;
    char            sql_sentence[1024];

    if (tuple == nullptr) {
        MSG_DBG("Parameter error");
        return false;
    }


    snprintf(sql_sentence, sizeof(sql_sentence),
        "INSERT INTO %s (dirpath, dirpath_hash, modify_time, update_flag, deleted_flag)\n"\
                "VALUES (?, ?, ?, ?, ?)\n"\
                "ON CONFLICT(dirpath) DO UPDATE SET\n"\
                "modify_time = excluded.modify_time,\n"\
                "update_flag = excluded.update_flag,\n"\
                "deleted_flag = excluded.deleted_flag;",
        _db_table_name);

    if (sqlite3_prepare_v2(get_db_p(), sql_sentence, -1, &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("SQLite prepare sentence failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }

    sqlite3_bind_text(sql_stmt, 1, dir_tuple->dirpath, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sql_stmt, 2, dir_tuple->dirpath_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(sql_stmt, 3, dir_tuple->modify_time);
    sqlite3_bind_int64(sql_stmt, 4, dir_tuple->update_flag);
    sqlite3_bind_int(sql_stmt, 5, dir_tuple->deleted_flag);

    if (sqlite3_step(sql_stmt) != SQLITE_DONE) {
        MSG_DBG_F("Execute table update failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }

    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return true;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return false;

}



bool Dir_table_controller::delete_tuple(const char * dir) {
    const char *    sql_sentence_str;
    char            sql_sentence[5096];
    char *          sql_error_msg = nullptr;
    sql_sentence_str =  "DELETE FROM %s\n"\
                        "WHERE dirpath = '%s';";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, dir);
    if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
        MSG_DBG_F("Delete tuple error: %s", sql_error_msg);
        goto failed;
    }
    if ( sql_error_msg != nullptr)
        sqlite3_free(sql_error_msg);

    return true;
failed:
    if ( sql_error_msg != nullptr)
        sqlite3_free(sql_error_msg);
    return false;
}

bool Dir_table_controller::mark_redundant_tuples() {
    const char *                sql_sentence_str;
    char                        sql_sentence[5096];
    sqlite3_stmt *              sql_stmt = nullptr;
    std::queue<std::string>     dir_qu;
    sql_sentence_str =  "SELECT dirpath\n"\
                        "FROM %s;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        const char * dir = (const char *)sqlite3_column_text(sql_stmt, 0);
        if ( access(dir, 0) != 0) {
            MSG_DBG_F("Donot find dir %s in system, delete it in table %s", dir, _db_table_name);
            dir_qu.emplace(dir);
        }
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("Sqlite step failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    while (!dir_qu.empty()) {
        MSG_DBG_F("Delete redundant dir '%s'", dir_qu.front().c_str());
        // if ( delete_tuple(files_qu.front().c_str()) == false) { // function in this class
        //     MSG_DBG_F("Delete failed");
        //     goto failed;
        // }
        memset(sql_sentence, 0, sizeof(sql_sentence));
        snprintf(sql_sentence, sizeof(sql_sentence),
                    "UPDATE %s\n"\
                    "SET deleted_flag = 1\n"\
                    "WHERE dirpath = ?;",
            _db_table_name
        );
        if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
            MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
            goto failed;
        }
        sqlite3_bind_text(sql_stmt, 1, dir_qu.front().c_str(), -1, SQLITE_TRANSIENT);
        if ( sqlite3_step(sql_stmt) != SQLITE_DONE) {
            MSG_DBG_F("SQL step failed: %s", sqlite3_errmsg(get_db_p()));
            // goto failed;
        }

        if (sql_stmt != nullptr) {
            sqlite3_finalize(sql_stmt);
            sql_stmt = nullptr;
        }

        dir_qu.pop();

    }

    return true;

failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
    }
    return false;
}

bool Dir_table_controller::remove_deleted_tuples() {
    const char *                sql_sentence_str;
    char                        sql_sentence[5096];
    sqlite3_stmt *              sql_stmt = nullptr;
    std::queue<std::string>     dir_qu;
    sql_sentence_str =  "SELECT dirpath\n"\
                        "FROM %s\n"\
                        "WHERE deleted_flag = 1;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        const char * filepath = (const char *)sqlite3_column_text(sql_stmt, 0);
        dir_qu.push(filepath);
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("Sqlite step failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    while (!dir_qu.empty()) {
        if ( delete_tuple(dir_qu.front().c_str()) == false) {
            MSG_DBG_F("delete dir '%s' failed", dir_qu.front().c_str());
        }

        dir_qu.pop();
    }


    return true;

failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return false;


}

void * Dir_table_controller::get_single_tuple(const char * dirpath) {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    dir_tuple_t *           table_tuple = nullptr;
    // std::queue<std::pair<std::string, std::string>> dir_qu;

    sql_sentence_str =  "SELECT dirpath, dirpath_hash, modify_time, update_flag, deleted_flag\n" \
                        "FROM %s\n"
                        "WHERE dirpath = ?;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    sqlite3_bind_text(sql_stmt, 1, dirpath, strlen(dirpath), SQLITE_TRANSIENT);

    table_tuple = (dir_tuple_t *)calloc(1, sizeof(dir_tuples_t) + sizeof(dir_tuple_t));
    if ( sqlite3_step(sql_stmt) == SQLITE_ROW) {
        // table_tuple->size = 1;
        strncpy(table_tuple->dirpath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(table_tuple->dirpath));
        strncpy(table_tuple->dirpath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(table_tuple->dirpath_hash));
        table_tuple->modify_time = sqlite3_column_int64(sql_stmt, 2);
        table_tuple->update_flag = sqlite3_column_int(sql_stmt, 3);
        table_tuple->deleted_flag = sqlite3_column_int(sql_stmt, 4);
    } else {
        free(table_tuple);
        table_tuple = nullptr;
    }

    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return table_tuple;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    if (table_tuple != nullptr) free(table_tuple);
    return nullptr;
}

void * Dir_table_controller::get_manage_dirs() {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    dir_tuples_t *           table_tuples = nullptr;

    sql_sentence_str =  "SELECT dirpath, dirpath_hash, modify_time, update_flag, deleted_flag\n" \
                        "FROM %s";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    // sqlite3_bind_text(sql_stmt, 1, dirpath, strlen(dirpath), SQLITE_TRANSIENT);

    table_tuples = (dir_tuples_t *)calloc(1, sizeof(dir_tuples_t) + sizeof(dir_tuple_t));
    if ( sqlite3_step(sql_stmt) == SQLITE_ROW) {
        table_tuples->size = 1;
        strncpy(table_tuples->tuples[0].dirpath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(table_tuples->tuples[0].dirpath));
        strncpy(table_tuples->tuples[0].dirpath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(table_tuples->tuples[0].dirpath_hash));
        table_tuples->tuples[0].modify_time = sqlite3_column_int64(sql_stmt, 2);
        table_tuples->tuples[0].update_flag = sqlite3_column_int(sql_stmt, 3);
        table_tuples->tuples[0].deleted_flag = sqlite3_column_int(sql_stmt, 4);
    } else {
        free(table_tuples);
        table_tuples = nullptr;
    }

    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return table_tuples;

failed:

    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    if (table_tuples != nullptr) free(table_tuples);
    return nullptr;
}


bool File_table_controller::bind(const char * database_path, const char * table_name) {
    const char *    sql_sentence_str;
    char            sql_sentence[5096];
    char *          sql_error_msg = nullptr;

    if (database_path == nullptr) {
        if (get_db_p() == nullptr) {
            MSG_DBG_F("failed to bind");
            return false;
        }
    } else {
        make_dir(database_path);
        // DB_controller only bind database, and Dir_table_controller only bind table_name
        if ( DB_controller::bind(database_path, nullptr) == false) {
            MSG_DBG_F("Database bind failed");
            return false;
        }
    }

    sql_sentence_str =  "CREATE TABLE IF NOT EXISTS %s (\n"\
                        "    filepath        TEXT        UNIQUE,\n"\
                        "    filepath_hash   TEXT        UNIQUE,\n"\
                        "    modify_time     BIGINT      NOT NULL,\n"\
                        "    file_size       BIGINT      NOT NULL,\n"\
                        "    is_file_or_dir  INT         NOT NULL,\n"\
                        "    update_flag     INT         NOT NULL,\n"\
                        "    deleted_flag    INT         NOT NULL,\n"\
                        "    PRIMARY KEY(filepath, filepath_hash)"\
                        ");";
    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, table_name);
    if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
        MSG_DBG_F("Create table %s error: %s", table_name, sql_error_msg);
        goto failed;
    }
    strncpy(_db_table_name, table_name, sizeof(_db_table_name));

    if ( sql_error_msg != nullptr)
        sqlite3_free(sql_error_msg);
    return true;
failed:
    if ( sql_error_msg != nullptr)
        sqlite3_free(sql_error_msg);
    return false;
}

void *  File_table_controller::get_tuples(int limit, int offset) {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    file_tuples_t *         file_tuples = nullptr;

    std::queue<file_tuple_t *> files_qu;
    int                     insert_pos = 0;

    sql_sentence_str =  "SELECT filepath, filepath_hash, modify_time, file_size, is_file_or_dir, update_flag, deleted_flag\n" \
                        "FROM %s\n"\
                        "LIMIT %d OFFSET %d;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, limit, offset);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        file_tuple_t *  file_tuple = (file_tuple_t *)calloc(1, sizeof(file_tuple_t));
        strncpy(file_tuple->filepath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(file_tuple->filepath));
        strncpy(file_tuple->filepath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(file_tuple->filepath_hash));
        file_tuple->modify_time = sqlite3_column_int64(sql_stmt, 2);
        file_tuple->file_size = sqlite3_column_int64(sql_stmt, 3);
        file_tuple->is_file_or_dir = sqlite3_column_int(sql_stmt, 4);
        file_tuple->update_flag = sqlite3_column_int(sql_stmt, 5);
        file_tuple->deleted_flag = sqlite3_column_int(sql_stmt, 6);

        files_qu.push(file_tuple);
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("SQLITE step error : %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    if ( files_qu.size() == 0) {
        file_tuples = nullptr;
    } else {
        file_tuples = (file_tuples_t *)calloc(1, sizeof(file_tuples_t) + files_qu.size() * sizeof(file_tuple_t));
        file_tuples->size = files_qu.size();

        while (!files_qu.empty()) {
            memcpy((char *)&file_tuples->tuples[insert_pos], (const char *)files_qu.front(), sizeof(file_tuple_t));
            insert_pos++;
            free(files_qu.front());
            files_qu.pop();
        }
    }

    return file_tuples;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return nullptr;
}


void *  File_table_controller::get_update_tuples(int limit, int offset) {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    file_tuples_t *         file_tuples = nullptr;

    std::queue<file_tuple_t *> files_qu;
    int                     insert_pos = 0;

    sql_sentence_str =  "SELECT filepath, filepath_hash, modify_time, file_size, is_file_or_dir, update_flag, deleted_flag\n" \
                        "FROM %s\n"\
                        "WHERE update_flag = 1\n"\
                        "LIMIT %d OFFSET %d;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, limit, offset);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        file_tuple_t *  file_tuple = (file_tuple_t *)calloc(1, sizeof(file_tuple_t));
        strncpy(file_tuple->filepath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(file_tuple->filepath));
        strncpy(file_tuple->filepath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(file_tuple->filepath_hash));
        file_tuple->modify_time = sqlite3_column_int64(sql_stmt, 2);
        file_tuple->file_size = sqlite3_column_int64(sql_stmt, 3);
        file_tuple->is_file_or_dir = sqlite3_column_int(sql_stmt, 4);
        file_tuple->update_flag = sqlite3_column_int(sql_stmt, 5);
        file_tuple->deleted_flag = sqlite3_column_int(sql_stmt, 6);

        files_qu.push(file_tuple);
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("SQLITE step error : %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    if ( files_qu.size() == 0) {
        file_tuples = nullptr;
    } else {
        file_tuples = (file_tuples_t *)calloc(1, sizeof(file_tuples_t) + files_qu.size() * sizeof(file_tuple_t));
        file_tuples->size = files_qu.size();

        while (!files_qu.empty()) {
            memcpy((char *)&file_tuples->tuples[insert_pos], (const char *)files_qu.front(), sizeof(file_tuple_t));
            insert_pos++;
            free(files_qu.front());
            files_qu.pop();
        }
    }

    return file_tuples;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return nullptr;

}

void *  File_table_controller::get_deleted_tuples(int limit, int offset) {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    file_tuples_t *         file_tuples = nullptr;

    std::queue<file_tuple_t *> files_qu;
    int                     insert_pos = 0;

    sql_sentence_str =  "SELECT filepath, filepath_hash, modify_time, file_size, is_file_or_dir, update_flag, deleted_flag\n" \
                        "FROM %s\n"\
                        "WHERE deleted_flag = 1\n"\
                        "LIMIT %d OFFSET %d;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name, limit, offset);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        file_tuple_t *  file_tuple = (file_tuple_t *)calloc(1, sizeof(file_tuple_t));
        strncpy(file_tuple->filepath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(file_tuple->filepath));
        strncpy(file_tuple->filepath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(file_tuple->filepath_hash));
        file_tuple->modify_time = sqlite3_column_int64(sql_stmt, 2);
        file_tuple->file_size = sqlite3_column_int64(sql_stmt, 3);
        file_tuple->is_file_or_dir = sqlite3_column_int(sql_stmt, 4);
        file_tuple->update_flag = sqlite3_column_int(sql_stmt, 5);
        file_tuple->deleted_flag = sqlite3_column_int(sql_stmt, 6);

        files_qu.push(file_tuple);
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("SQLITE step error : %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    if ( files_qu.size() == 0) {
        file_tuples = nullptr;
    } else {
        file_tuples = (file_tuples_t *)calloc(1, sizeof(file_tuples_t) + files_qu.size() * sizeof(file_tuple_t));
        file_tuples->size = files_qu.size();

        while (!files_qu.empty()) {
            memcpy((char *)&file_tuples->tuples[insert_pos], (const char *)files_qu.front(), sizeof(file_tuple_t));
            insert_pos++;
            free(files_qu.front());
            files_qu.pop();
        }
    }

    return file_tuples;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return nullptr;
}

bool  File_table_controller::update_table(void * tuple) {
    file_tuple_t *  file_tuple = (file_tuple_t *)tuple;
    sqlite3_stmt *  sql_stmt = nullptr;
    char            sql_sentence[1024];

    if (tuple == nullptr) {
        MSG_DBG("Parameter error");
        return false;
    }


    snprintf(sql_sentence, sizeof(sql_sentence),
        "INSERT INTO %s (filepath, filepath_hash, modify_time, file_size, is_file_or_dir, update_flag, deleted_flag)\n"\
                "VALUES (?, ?, ?, ?, ?, ?, ?)\n"\
                "ON CONFLICT(filepath) DO UPDATE SET\n"\
                "modify_time = excluded.modify_time,\n"\
                "file_size = excluded.file_size,\n"\
                "is_file_or_dir = excluded.is_file_or_dir,\n"\
                "update_flag = excluded.update_flag,\n"\
                "deleted_flag = excluded.deleted_flag;",
        _db_table_name);

    if (sqlite3_prepare_v2(get_db_p(), sql_sentence, -1, &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("SQLite prepare sentence failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }

    sqlite3_bind_text(sql_stmt, 1, file_tuple->filepath, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(sql_stmt, 2, file_tuple->filepath_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(sql_stmt, 3, file_tuple->modify_time);
    sqlite3_bind_int64(sql_stmt, 4, file_tuple->file_size);
    sqlite3_bind_int(sql_stmt, 5, file_tuple->is_file_or_dir);
    sqlite3_bind_int(sql_stmt, 6, file_tuple->update_flag);
    sqlite3_bind_int(sql_stmt, 7, file_tuple->deleted_flag);

    if (sqlite3_step(sql_stmt) != SQLITE_DONE) {
        MSG_DBG_F("Execute table update failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }

    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return true;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return false;
}


bool File_table_controller::delete_tuple(const char * filepath) {


    sqlite3_stmt * sql_stmt = nullptr;
    char sql_sentence[1024];


    snprintf(sql_sentence, sizeof(sql_sentence),
                    "DELETE FROM %s\n"\
                            "WHERE filepath = ?;",
            _db_table_name);

    // if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
    //     MSG_DBG_F("Delete tuple error: %s", sql_error_msg);
    //     goto failed;
    // }
    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("SQLite prepare sentence failed : %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }

    sqlite3_bind_text(sql_stmt, 1, filepath, strlen(filepath), SQLITE_TRANSIENT);

    if ( (sqlite3_step(sql_stmt)) != SQLITE_DONE) {
        MSG_DBG_F("Update table failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return true;
failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return false;
}

bool File_table_controller::mark_redundant_tuples() {
    const char *                sql_sentence_str;
    char                        sql_sentence[5096];
    sqlite3_stmt *              sql_stmt = nullptr;
    std::queue<std::string>     files_qu;
    sql_sentence_str =  "SELECT filepath\n"\
                        "FROM %s;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        const char * filepath = (const char *)sqlite3_column_text(sql_stmt, 0);
        if ( access(filepath, 0) != 0) {
            MSG_DBG_F("Donot find dir %s in system, delete it in table %s", filepath, _db_table_name);
            files_qu.emplace(filepath);
        }
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("Sqlite step failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    while (!files_qu.empty()) {
        MSG_DBG_F("Mark redundant file '%s'", files_qu.front().c_str());
        // if ( delete_tuple(files_qu.front().c_str()) == false) { // function in this class
        //     MSG_DBG_F("Delete failed");
        //     goto failed;
        // }
        memset(sql_sentence, 0, sizeof(sql_sentence));
        snprintf(sql_sentence, sizeof(sql_sentence),
                    "UPDATE %s\n"\
                    "SET deleted_flag = 1\n"\
                    "WHERE filepath = ?;",
            _db_table_name
        );
        if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
            MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
            goto failed;
        }
        sqlite3_bind_text(sql_stmt, 1, files_qu.front().c_str(), -1, SQLITE_TRANSIENT);
        if ( sqlite3_step(sql_stmt) != SQLITE_DONE) {
            MSG_DBG_F("SQL step failed: %s", sqlite3_errmsg(get_db_p()));
            // goto failed;
        }

        files_qu.pop();
    }


    return true;

failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return false;
}


bool File_table_controller::remove_deleted_tuples() {
    const char *                sql_sentence_str;
    char                        sql_sentence[5096];
    sqlite3_stmt *              sql_stmt = nullptr;
    std::queue<std::string>     files_qu;
    sql_sentence_str =  "SELECT filepath\n"\
                        "FROM %s\n"\
                        "WHERE deleted_flag = 1;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    int sql_rtn;
    while ( (sql_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        const char * filepath = (const char *)sqlite3_column_text(sql_stmt, 0);
        files_qu.push(filepath);
    }
    if ( sql_rtn != SQLITE_DONE) {
        MSG_DBG_F("Sqlite step failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }

    while (!files_qu.empty()) {
        MSG_DBG_F("delete file '%s'", files_qu.front().c_str());
        if ( delete_tuple(files_qu.front().c_str()) == false) {
            MSG_DBG_F("delete file '%s' failed", files_qu.front().c_str());
        }

        files_qu.pop();
    }


    return true;

failed:
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return false;
}


void * File_table_controller::get_single_tuple(const char * filepath) {
    const char *            sql_sentence_str;
    char                    sql_sentence[5096];
    sqlite3_stmt *          sql_stmt = nullptr;
    file_tuple_t *          file_tuple = nullptr;

    if ( (file_tuple = (file_tuple_t *)calloc(1, sizeof(file_tuple_t))) == NULL) {
        MSG_DBG_F("Error: %s", strerror(errno));
        goto failed;
    }
    sql_sentence_str =  "SELECT filepath, filepath_hash, modify_time, file_size, is_file_or_dir, update_flag, deleted_flag\n" \
                        "FROM %s\n"\
                        "WHERE filepath = ?;";

    memset(sql_sentence, 0, sizeof(sql_sentence));
    snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);

    if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
        MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
        goto failed;
    }
    sqlite3_bind_text(sql_stmt, 1, filepath, -1, SQLITE_STATIC);
    int step_rtn;
    if ( (step_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        strncpy(file_tuple->filepath, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(file_tuple->filepath));
        strncpy(file_tuple->filepath_hash, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(file_tuple->filepath_hash));
        file_tuple->modify_time = sqlite3_column_int64(sql_stmt, 2);
        file_tuple->file_size = sqlite3_column_int64(sql_stmt, 3);
        file_tuple->is_file_or_dir = sqlite3_column_int(sql_stmt, 4);
        file_tuple->update_flag = sqlite3_column_int(sql_stmt, 5);
        file_tuple->deleted_flag = sqlite3_column_int(sql_stmt, 6);
    } else {
        if ( step_rtn == SQLITE_DONE) {
            free(file_tuple);
            file_tuple = nullptr;
            MSG_DBG_F("filepath '%s' is not in table, return nullptr", filepath);
        } else {
            MSG_DBG_F("SQLITE step error : %s", sqlite3_errmsg(get_db_p()));
            goto failed;
        }
    }
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return file_tuple;

failed:
    if (file_tuple != nullptr)
        free(file_tuple);
    if (sql_stmt != nullptr) {
        sqlite3_finalize(sql_stmt);
        sql_stmt = nullptr;
    }
    return nullptr;
}



// bool Config_table_controller::bind(const char * database_path, const char * table_name) {
//     const char *    sql_sentence_str;
//     char            sql_sentence[5096];
//     char *          sql_error_msg = nullptr;
//
//     // make_dir(database_path);
//     // DB_controller only bind database, and Dir_table_controller only bind table_name
//     if ( DB_controller::bind(database_path, nullptr) == false) {
//         MSG_DBG_F("Database bind failed");
//         return false;
//     }
//
//     sql_sentence_str =  "CREATE TABLE IF NOT EXISTS %s (\n"\
//                         "    KEY    TEXT     NOT NULL,\n"\
//                         "    VALUE  TEXT     NOT NULL,\n"\
//                         "    PRIMARY KEY(KEY)\n"\
//                         ");";
//     memset(sql_sentence, 0, sizeof(sql_sentence));
//     snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, table_name);
//     if ( sqlite3_exec(get_db_p(), sql_sentence, NULL, NULL, &sql_error_msg) != SQLITE_OK) {
//         MSG_DBG_F("Create table %s error: %s", table_name, sql_error_msg);
//         goto failed;
//     }
//     strncpy(_db_table_name, table_name, sizeof(_db_table_name));
//
//     if ( sql_error_msg != nullptr)
//         sqlite3_free(sql_error_msg);
//     return true;
// failed:
//     if ( sql_error_msg != nullptr)
//         sqlite3_free(sql_error_msg);
//     return false;
// }
//
//
// bool Config_table_controller::update_table(void * tuple) {
//     config_tuple_t *config_tuple = (config_tuple_t *)tuple;
//     sqlite3_stmt *  sql_stmt = nullptr;
//     char            sql_sentence[1024];
//
//     if (tuple == nullptr) {
//         MSG_DBG("Parameter error");
//         return false;
//     }
//
//
//     snprintf(sql_sentence, sizeof(sql_sentence),
//         "INSERT INTO %s (key, value)\n"\
//                 "VALUES (?, ?)\n"\
//                 "ON CONFLICT(key) DO UPDATE SET\n"\
//                 "value = excluded.value;",
//         _db_table_name);
//
//     if (sqlite3_prepare_v2(get_db_p(), sql_sentence, -1, &sql_stmt, NULL) != SQLITE_OK) {
//         MSG_DBG_F("SQLite prepare sentence failed: %s", sqlite3_errmsg(get_db_p()));
//         goto failed;
//     }
//
//     sqlite3_bind_text(sql_stmt, 1, config_tuple->key, -1, SQLITE_TRANSIENT);
//     sqlite3_bind_text(sql_stmt, 2, config_tuple->value, -1, SQLITE_TRANSIENT);
//
//     if (sqlite3_step(sql_stmt) != SQLITE_DONE) {
//         MSG_DBG_F("Execute table update failed: %s", sqlite3_errmsg(get_db_p()));
//         goto failed;
//     }
//
//     if (sql_stmt != nullptr) {
//         sqlite3_finalize(sql_stmt);
//         sql_stmt = nullptr;
//     }
//     return true;
// failed:
//     if (sql_stmt != nullptr) {
//         sqlite3_finalize(sql_stmt);
//         sql_stmt = nullptr;
//     }
//     return false;
// }
//
//
// void * Config_table_controller::get_single_tuple(const char * key) {
//     const char *            sql_sentence_str;
//     char                    sql_sentence[5096];
//     sqlite3_stmt *          sql_stmt = nullptr;
//     config_tuple_t *        config_tuple = nullptr;
//
//     if ( (config_tuple = (config_tuple_t *)calloc(1, sizeof(config_tuple_t))) == NULL) {
//         MSG_DBG_F("Error: %s", strerror(errno));
//         goto failed;
//     }
//     sql_sentence_str =  "SELECT key, value\n" \
//                         "FROM %s\n"\
//                         "WHERE key = ?;";
//
//     memset(sql_sentence, 0, sizeof(sql_sentence));
//     snprintf(sql_sentence, sizeof(sql_sentence), sql_sentence_str, _db_table_name);
//
//     if ( sqlite3_prepare_v2(get_db_p(), sql_sentence, strlen(sql_sentence), &sql_stmt, NULL) != SQLITE_OK) {
//         MSG_DBG_F("Sqlite prepare failed: %s", sqlite3_errmsg(get_db_p()));
//         goto failed;
//     }
//     sqlite3_bind_text(sql_stmt, 1, key, -1, SQLITE_STATIC);
//     int step_rtn;
//     if ( (step_rtn = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
//         strncpy(config_tuple->key, (const char *)sqlite3_column_text(sql_stmt, 0), sizeof(config_tuple->key));
//         strncpy(config_tuple->value, (const char *)sqlite3_column_text(sql_stmt, 1), sizeof(config_tuple->value));
//     } else {
//         if ( step_rtn == SQLITE_DONE) {
//             free(config_tuple);
//             config_tuple = nullptr;
//             MSG_DBG_F("Key '%s' is not in table, return nullptr", key);
//         } else {
//             MSG_DBG_F("SQLITE step error : %s", sqlite3_errmsg(get_db_p()));
//             goto failed;
//         }
//     }
//     if (sql_stmt != nullptr) {
//         sqlite3_finalize(sql_stmt);
//         sql_stmt = nullptr;
//     }
//     return config_tuple;
//
// failed:
//     if (config_tuple != nullptr)
//         free(config_tuple);
//     if (sql_stmt != nullptr) {
//         sqlite3_finalize(sql_stmt);
//         sql_stmt = nullptr;
//     }
//     return nullptr;
// }
//
