#pragma once
#include "deliver/deliver.h"

#include "table_control.hpp"
#include <cstdint>
#include <list>
#include <spawn.h>
#include <string>

extern "C" {
    #include <pthread.h>
}


// // The data which client should send to server, and server need to parse it to compare local database;
// // After server compare all the entrys,
// // he should send data which type is database_entrys_need_t to request send_single_file process about specific file from client.
// typedef struct dir_entry_set {
//     // int     de_entry_size;
//     char    de_hash_names[65];
//     int     de_modify_times;
//     int     de_file_sizes;
//     short   de_file_flags;
// } dir_entry_set_t;
//
// // for server(receive process), which needs to send the hash names to request client to send files
// typedef struct dir_entrys_need {
//     int     den_entry_size;
//     char    den_hash_names[65][100];
// } dir_entrys_need_t;
//
// typedef struct entrys_hashname {
//     char hash_names[65];
// } entrys_hashname_t;
//

enum deliver_dir_tag : uint8_t {
    DHT_DIR_INFORMATION,
    DHT_DIR_SEND_FILE,      // before call send_single_file, sender need to use it
    // DHT_DIR_SENDING,
    DHT_DIR_FINISH_SENDING,
    DHT_DIR_SEND_END,
    DHT_DIR_SEND_TUPLES,
    // DHT_DIR_SEND_TUPLES_DONE,
    DHT_DIR_FILE_NEED,      // need to add file_need_list
};


typedef enum : uint8_t {
    DI_NO_NEED_DELIVER = 0x01,
    DI_FAILED          = 0x02,
    DI_NEED_COMPARE    = 0x03,
    DI_NEED_ALL        = 0x04
} di_stat_t;

typedef struct {
    di_stat_t   di_stat;
    // char        fi_msg[20];     // 如果失败, 则此成员返回字符串格式失败信息
} dir_info_ack_t;

class Sync_dir_sender {
public:
    // config sockfd and socktag of deliver series functions,
    // create Sender_map_table, if it is not exist in db
    Sync_dir_sender(int sockfd, uint16_t deliver_file_port, sock_tag_t socktag, const char * dirpath_from_database, const char * verify_rootpath, pid_t leader_pid);
    virtual bool sync_start();
    Sync_dir_sender() = delete;
    ~Sync_dir_sender();
private:
    virtual bool send_info();
    virtual bool get_info_ack();
    virtual bool send_unit();
    virtual bool send_unit_entrys();
    virtual bool recv_unit_need();
    virtual bool send_unit_need();
// private:
//     bool generate_info(int update_flag);
private:
    Dir_table_controller    dir_table_controller;
    File_table_controller   file_table_controller;
    char                    dirpath_from_database[4096];
    int                     sockfd;
    // int                     file_sockfd;
    uint16_t                deliver_file_port;
    sock_tag_t              socktag;
    // char                    dest_address[255];
    size_t                  message_len;
    int                     message_empty_flag;
    dir_info_ack_t          info_ack;
    // int                     send_info_flag;
    // int                     entry_len_set;
    // dir_entry_set_t         entry_set;
    // char                    dir_path[4096];
    uint16_t                basepath_len = 0;
    char                    basepath_hash[65];
    char                    verify_rootpath[4096];
    char                    cur_verify_dirpath[4096];
    char                    db_path[4096];


    std::list<std::string>  file_send_list;

    pid_t                   leader_pid;

    // pthread_mutex_t         list_mutex;
    // pthread_cond_t          list_cond;
};

class Sync_dir_receiver {
public:
    Sync_dir_receiver(int sockfd, sock_tag_t socktag, pid_t leader_pid);
    virtual bool sync_start();
    Sync_dir_receiver() = delete;
    ~Sync_dir_receiver();
private:
    virtual bool recv_info();
    virtual bool send_info_ack();
    virtual bool recv_unit();
    virtual bool recv_unit_and_compare();
    virtual bool send_unit_need();
    virtual bool recv_unit_need();

private:
    Dir_table_controller    dir_table_controller;
    File_table_controller   file_table_controller;

    dir_tuple_t *          dir_tuple;
    // file_tuples_t *         file_tuples;

    int                     sockfd;
    int                     file_sockfd;
    sock_tag_t              socktag;
    size_t                  message_len;
    int                     message_empty_flag;
    dir_info_ack_t          info_ack;

    // typedef struct {
    //     std::string     filepath;
    //     file_tuple_t    file_tuple;
    // } file_need_node_t;
    std::list<std::string>  file_need_list;
    // char                    recv_rootpath[4096],    cur_recv_dirpath[4096];
    // char                    verify_rootpath[4096],  cur_verify_dirpath[4096];


    pid_t                   leader_pid;
};
