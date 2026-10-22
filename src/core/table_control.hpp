// For all program which need to use database.
// they usually use "init" function to create some tables,
// and 'remove' function to remove the tuple corresponding to the non-existent dir

#pragma once
#include <cstdint>

extern "C" {
    #include "sqlite/sqlite3.h"
}

class DB_controller {
public:
    // Create db and table (depending on the situation)
    DB_controller() = default;

    virtual bool bind(const char * dabase_path, const char * table_name);

    virtual void *  get_tuples(int limit, int offset) { return nullptr; };
    virtual void *  get_update_tuples(int limit, int offset) { return nullptr; };
    virtual void *  get_deleted_tuples(int limit, int offset) { return nullptr; };

    static  void    free_tuples(void * tuples) ;

    // virtual bool    tuple_is_exist(const char * select_sql) = 0;

    virtual bool    update_table(void * tuple) { return false; };

    virtual bool    delete_tuple(const char *) { return false; };

    // Tarverse all tuples,
    // if the dir or file does not exist in system, remove tuple of it
    virtual bool    mark_redundant_tuples() { return false; };

    virtual bool    remove_deleted_tuples() { return false; };

    virtual void *  get_single_tuple(const char *) { return nullptr; };
protected:
    sqlite3 *       get_db_p() {
        return _db;
    }

private:
    // const char *     _sql_sentence_str;
    // char             _sql_sentence[4096];
    // sqlite3_stmt *   _sql_stmt;
    // char *           _sql_error_msg;
    static sqlite3 * _db;

};


// typedef struct {
//     int     size;
//     char    dirpath[100][4096];
//     char    dirpath_hash[100][65];
// } dir_tuples_t;
#pragma pack(1)
typedef struct {
    char        dirpath[4096];
    char        dirpath_hash[65];
    uint64_t    modify_time;
    uint16_t    update_flag;
    uint16_t    deleted_flag;
} dir_tuple_t;

typedef struct {
    uint32_t        size;
    dir_tuple_t     tuples[];
} dir_tuples_t;
#pragma pack()

class Dir_table_controller : public DB_controller {
public:
    Dir_table_controller() = default;

    virtual bool    bind(const char * dabase_path, const char * table_name);

    virtual void *  get_tuples(int limit, int offset);
    virtual void *  get_update_tuples(int limit, int offset);
    virtual void *  get_deleted_tuples(int limit, int offset);
    // virtual void    free_tuples(void * tuples);

    // virtual bool    tuple_is_exist(const char * select_sql);

    virtual bool    update_table(void * tuple);

    virtual bool    delete_tuple(const char * dirpath);

// protected:
//     const char *    get_table_name() {
//         return _db_table_name;
//     }
    // Tarverse all tuples,
    // if the dir or file does not exist in system, remove tuple of it
    virtual bool    mark_redundant_tuples();

    virtual bool    remove_deleted_tuples();

    virtual void *  get_single_tuple(const char * dirpath);

    void * get_manage_dirs();
private:

    char             _db_table_name[100];

};

#pragma pack(1)
typedef struct {
    char        filepath[4096];
    char        filepath_hash[65];
    uint64_t    modify_time;
    uint64_t    file_size;
    uint16_t    is_file_or_dir;
    uint16_t    update_flag;
    uint16_t    deleted_flag;
} file_tuple_t;

typedef struct {
    uint32_t       size;
    file_tuple_t   tuples[];
} file_tuples_t;
#pragma pack()

class File_table_controller : public DB_controller {
public:
    File_table_controller() = default;

    virtual bool    bind(const char * dabase_path, const char * table_name);

    virtual void *  get_tuples(int limit, int offset);
    virtual void *  get_update_tuples(int limit, int offset);
    virtual void *  get_deleted_tuples(int limit, int offset);

    // virtual void    free_tuples(void * tuples);

    // virtual bool    tuple_is_exist(const char * select_sql);

    virtual bool    update_table(void * tuple);

    virtual bool    delete_tuple(const char * filepath);


    // Tarverse all tuples,
    // if the dir or file does not exist in system, remove tuple of it
    virtual bool    mark_redundant_tuples();

    virtual bool    remove_deleted_tuples();

    virtual void *  get_single_tuple(const char * filepath);
private:
    char             _db_table_name[100];
};




// typedef struct {
//     char key[4096];
//     char value[4096];
// } config_tuple_t;
//
// class Config_table_controller : public DB_controller {
// public:
//     Config_table_controller() = default;
//
//     virtual bool    bind(const char * dabase_path, const char * table_name);
//
//     // virtual void *  get_tuples(int limit, int offset);
//     // virtual void *  get_update_tuples(int limit, int offset);
//     // virtual void *  get_deleted_tuples(int limit, int offset);
//
//     // virtual void    free_tuples(void * tuples);
//
//     // virtual bool    tuple_is_exist(const char * select_sql);
//
//     virtual bool    update_table(void * tuple);
//
//     // virtual bool    delete_tuple(const char * key);
//
//
//     // Tarverse all tuples,
//     // if the dir or file does not exist in system, remove tuple of it
// //     virtual bool    mark_redundant_tuples();
// //
// //     virtual bool    remove_deleted_tuples();
//
//     virtual void *  get_single_tuple(const char * key);
// private:
//     char             _db_table_name[100];
// };
