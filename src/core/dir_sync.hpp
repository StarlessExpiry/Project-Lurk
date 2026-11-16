#pragma once
#include "deliver.h"

#include "table_control.hpp"
#include <cstdint>
#include <cstring>
#include <list>
#include <string.h>
#include <pthread.h>

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


class Sync_dir_sender {
    friend class Sync_dir_sender_builder;
    friend class DirSendInfoState;
    friend class DirSendFilesState;
    friend class DirSendCompareState;
    friend class DirSendFilesNeededState;
    friend class DirSendCloseState;
public:
    bool sync_start();
    ~Sync_dir_sender();
private:
    Sync_dir_sender() {};
private:
    Dir_table_controller    dir_table_controller;
    File_table_controller   file_table_controller;
    char                    dirpath_from_database[4096];
    int                     sockfd;
    uint16_t                deliver_file_port;
    sock_tag_t              socktag;
    uint16_t                basepath_len = 0;
    char                    basepath_hash[65];
    char                    verify_rootpath[4096];
    char                    db_path[4096];
    pid_t                   leader_pid;
};

class Sync_dir_sender_builder {
public:
    Sync_dir_sender_builder() : sender(new Sync_dir_sender) {};

    Sync_dir_sender_builder& set_sockfd(int sockfd) {
        sender->sockfd = sockfd;
        return *this;
    }
    Sync_dir_sender_builder& set_deliver_port(uint16_t deliver_file_port) {
        sender->deliver_file_port = deliver_file_port;
        return *this;
    }
    Sync_dir_sender_builder& set_dirpath_from_database(const char * dirpath_from_database) {
        strncpy(sender->dirpath_from_database, dirpath_from_database, sizeof(sender->dirpath_from_database));
        return *this;
    }
    Sync_dir_sender_builder& set_verify_rootpath(const char * verify_rootpath) {
        strncpy(sender->verify_rootpath, verify_rootpath, sizeof(sender->verify_rootpath));
        return *this;
    }
    Sync_dir_sender_builder& set_leader_pid(pid_t leader_pid) {
        sender->leader_pid = leader_pid;
        return *this;
    }
    Sync_dir_sender * build() {
        return sender;
    }

private:
    Sync_dir_sender * sender;
};

class Sync_dir_receiver {
    friend class Sync_dir_receiver_builder;
    friend class DirRecvState;
    friend class DirRecvInfoState;
    friend class DirRecvFilesState;
    friend class DirRecvCompareState;
    friend class DirRecvFilesNeededState;
    friend class DirRecvCloseState;
public:
    bool sync_start();
private:
    Sync_dir_receiver() {};
private:
    Dir_table_controller    dir_table_controller;
    File_table_controller   file_table_controller;
    int                     sockfd;
    dir_tuple_t *           dir_tuple;
    int                     file_sockfd;
    sock_tag_t              socktag;
    size_t                  message_len;
    int                     message_empty_flag;
    std::list<std::string>  file_need_list;
    pid_t                   leader_pid;
};

class Sync_dir_receiver_builder {
public:
    Sync_dir_receiver_builder() : receiver(new Sync_dir_receiver) {}

    Sync_dir_receiver_builder& set_sockfd(int sockfd) {
        receiver->sockfd = sockfd;
        return *this;
    }
    Sync_dir_receiver_builder& set_socktag(sock_tag_t socktag) {
        receiver->socktag = socktag;
        return *this;
    }
    Sync_dir_receiver_builder& set_leader_pid(pid_t leader_pid) {
        receiver->leader_pid = leader_pid;
        return *this;
    }
    Sync_dir_receiver * build() {
        return receiver;
    }
private:
    Sync_dir_receiver * receiver;
};
