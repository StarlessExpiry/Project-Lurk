#include "dir_sync.hpp"
#include "deliver/deliver.h"
#include "table_control.hpp"
#include "tools/get_config.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stack>


extern "C" {
    #include "sys/msg_print.h"
    #include "tools/makedir.h"
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
    #include <spawn.h>
}

// system environment, read only
extern char ** environ;

bool send_single_file_process(uint16_t deliver_file_port, sock_tag_t socktag, char * filepath, int basepath_len, char basepath_hash[65], char *verify_dir) {
    posix_spawn_file_actions_t      process_action; // Redirect the send_single_file process's stdout to the current process's stdout/stderror
    posix_spawnattr_t               process_attr; // enable the process join a same process group, it is convenient to kill all of them
    posix_spawn_file_actions_init(&process_action);

    posix_spawnattr_init(&process_attr);
    posix_spawnattr_setflags(&process_attr, POSIX_SPAWN_SETPGROUP);
    posix_spawnattr_setpgroup(&process_attr, getpid());

    posix_spawn_file_actions_adddup2(&process_action, STDOUT_FILENO, STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&process_action, STDERR_FILENO, STDERR_FILENO);


    char basepath_len_str[10];
    memset(basepath_len_str, 0, sizeof(basepath_len_str));
    snprintf(basepath_len_str, sizeof(basepath_len_str), "%d", basepath_len);

    char * argv[8];
    argv[0] = (char *)"lurk_client_single_file_deliver";
    if (socktag == ST_COMMON) {
        argv[1] = (char *)"ST_COMMON";
    }
    char        deliver_file_port_str[10];
    memset(deliver_file_port_str, 0, sizeof(deliver_file_port_str));
    snprintf(deliver_file_port_str, sizeof(deliver_file_port_str), "%hu", deliver_file_port);

    argv[2] = deliver_file_port_str;
    argv[3] = filepath;
    argv[4] = basepath_len_str;
    argv[5] = basepath_hash;
    argv[6] = verify_dir;
    argv[7] = NULL; // null terminated

    pid_t pid_rtn;
    if ( posix_spawn(&pid_rtn, "./lurk_client_single_file_deliver", &process_action, &process_attr, argv, environ) != 0) {
        MSG_DBG_F("create process errro : %s", strerror(errno));
        posix_spawnattr_destroy(&process_attr);
        posix_spawn_file_actions_destroy(&process_action);
        return false;
    }
    MSG_DBG_F("create process success, pid = %d", pid_rtn);

    posix_spawnattr_destroy(&process_attr);
    posix_spawn_file_actions_destroy(&process_action);

    // wait the send_file_process
    int status;
    waitpid(pid_rtn, &status, 0);

    if (WIFEXITED(status)) {
        MSG_DBG_F("Process exit with stat: %d", WEXITSTATUS(status));
        int exit_code = WEXITSTATUS(status);
        if (exit_code == EXIT_SUCCESS) {
            // MSG_DBG_F("Process return EXIT_SUCCESS");
            // printf("[-send_single_file_process-]: send file '%s' success.\n", filepath);
        } else {
            // MSG_DBG_F("Process return EXIT_FAILURE");
            printf("[send single file process]: send file '%s' failed.\n", filepath);
            return false;
        }
    } else if (WIFSIGNALED(status)) {
        printf("[send single file process]: send file '%s' process is killed with signal %d (SIGINT is 2, SIGEGV is 11)\n", filepath, WTERMSIG(status));
        return false;
    }

    return true;
}

// bool recv_single_file_process(posix_spawnattr_t & attr,
//                         sock_tag_t socktag, char * recv_path, char * verify_path, pid_t * pid_rtn) {
//
//     char * argv[5];
//     argv[0] = (char *)"lurk_server_single_file_deliver";
//     if (socktag == ST_COMMON) {
//         argv[1] = (char *)"ST_COMMON";
//     }
//     argv[2] = recv_path;
//     argv[3] = verify_path;
//     argv[4] = NULL;
//
//     if ( posix_spawn(pid_rtn, "./lurk_server_single_file_deliver", NULL, &attr, argv, environ) != 0) {
//         MSG_DBG_F("create process errro : %s", strerror(errno));
//         return false;
//     }
//     MSG_DBG_F("create process success, pid = %d", *pid_rtn);
//     return true;
// }

Sync_dir_sender::Sync_dir_sender(int sockfd, uint16_t deliver_file_port, sock_tag_t socktag, const char * dirpath_from_database, const char * verify_rootpath, pid_t leader_pid) {
    this->sockfd = sockfd;
    this->deliver_file_port = deliver_file_port;
    this->socktag = socktag;

    if (leader_pid == 0) {
        this->leader_pid = getpid();
    }
    memset(this->dirpath_from_database, 0, sizeof(this->dirpath_from_database));
    strncpy(this->dirpath_from_database, dirpath_from_database, sizeof(this->dirpath_from_database));

    // MSG_DBG_F("DIR '%s' sync process is preparing, pid = %d, leader_pid = %d", dirpath_from_database, (int)getpid(), leader_pid);

    // ~/.cache/lurk
    memset(this->verify_rootpath, 0, sizeof(this->verify_rootpath));
    memcpy(this->verify_rootpath, verify_rootpath, strlen(verify_rootpath));
    make_dir(verify_rootpath);

    memset(db_path, 0, sizeof(db_path));
    snprintf(db_path, sizeof(db_path), "%s/%s", verify_rootpath, "lurk_database.db");


    if ( dir_table_controller.bind(db_path, "Sender_map_table") == false)
        MSG_DBG_F("controller bind failed");
}
Sync_dir_sender::~Sync_dir_sender() {
    // posix_spawnattr_destroy(&process_attr);
}


Sync_dir_receiver::Sync_dir_receiver(int sockfd, sock_tag_t socktag, pid_t leader_pid) {
    this->sockfd = sockfd;
    this->socktag = socktag;

//     memset(this->recv_rootpath, 0, sizeof(this->recv_rootpath));
//     strncpy(this->recv_rootpath, dir_to_receive, strlen(dir_to_receive));
//     this->recv_rootpath[strlen(this->recv_rootpath)] = '/';
//     make_dir(this->recv_rootpath);
//     this->recv_rootpath[strlen(this->recv_rootpath)-1] = 0;
//
//     memset(this->verify_rootpath, 0, sizeof(this->verify_rootpath));
//     strncpy(this->verify_rootpath, verify_rootpath, strlen(verify_rootpath));
//     this->verify_rootpath[strlen(this->verify_rootpath)] = '/';
//     make_dir(this->verify_rootpath);
//     this->verify_rootpath[strlen(this->verify_rootpath)-1] = 0;

    char        verify_dir[4096];
    char        db_path[4096];
    memset(verify_dir, 0, sizeof(verify_dir));
    get_config_verifypath_server(verify_dir);
    memset(db_path, 0, sizeof(db_path));
    snprintf(db_path, sizeof(db_path), "%s/%s", verify_dir, "lurk_database.db");


    if ( dir_table_controller.bind(db_path, "Receiver_map_table") == false)
        MSG_DBG_F("controller bind failed");

}
Sync_dir_receiver::~Sync_dir_receiver() {
    // posix_spawnattr_destroy(&process_attr);
}


typedef enum {
    PROCESS_DELIVER_INFO,
    PROCESS_DELIVER_FILES,
    PROCESS_DELIVER_TUPLES,
    PROCESS_DELIVER_FILE_NEED
} PROCESS_INDICATOR_T;

bool Sync_dir_sender::sync_start() {
    PROCESS_INDICATOR_T     process_indicator;
    int                     send_flag, read_flag;

    // === for select
    int                     maxfd = sockfd;
    fd_set                  rset, wset;
    int                     nready;
    process_indicator = PROCESS_DELIVER_INFO;
    send_flag = 1, read_flag = 0;

    while (1) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        if (process_indicator == PROCESS_DELIVER_INFO && send_flag == 1)
            FD_SET(sockfd, &wset);
        if (process_indicator == PROCESS_DELIVER_INFO && read_flag == 1)
            FD_SET(sockfd, &rset);
        if (process_indicator == PROCESS_DELIVER_FILES)
            FD_SET(sockfd, &wset);
        if (process_indicator == PROCESS_DELIVER_TUPLES)
            FD_SET(sockfd, &wset);
        if (process_indicator == PROCESS_DELIVER_FILE_NEED && send_flag == 1)
            FD_SET(sockfd, &wset);
        if (process_indicator == PROCESS_DELIVER_FILE_NEED && read_flag == 1)
            FD_SET(sockfd, &rset);

        MSG_DBG_F("Dir Select wait, fd = %d", sockfd);
        if ( (nready = select(maxfd+1, &rset, &wset, NULL, NULL)) == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                MSG_DBG_F("select error");
                perror("[dir sync process]");
                goto failed;
            }
        }
        if (FD_ISSET(sockfd, &wset)) {
            switch (process_indicator) {
                case PROCESS_DELIVER_INFO: {
                    MSG_DBG_F("DIR: Turn into PROCESS_HANDLE_INFO (SEND)");
                    if ( send_info() == false) {
                        MSG_DBG_F("send_info failed");
                        goto failed;
                    }

                    send_flag = 0, read_flag = 1;
                    break;
                }
                case PROCESS_DELIVER_FILES: {
                    MSG_DBG("DIR: Turn into PROCESS_SEND_FILES");
                    if (send_unit() == false) {
                        MSG_DBG("send unit failed");
                        printf("send file failed\n");
                        goto failed;
                    }
                    goto success;
                    break;
                }
                case PROCESS_DELIVER_TUPLES: {
                    MSG_DBG("DIR: Turn into PROCESS_SEND_TUPLES");
                    // all entrys
                    if ( send_unit_entrys() == false) {
                        MSG_DBG("send unit entrys failed");
                        goto failed;
                    }

                    process_indicator = PROCESS_DELIVER_FILE_NEED;
                    read_flag = 1, send_flag = 0;
                    break;
                }
                case PROCESS_DELIVER_FILE_NEED: {
                    MSG_DBG_F("DIR: Turn into PROCESS_HANDLE_FILE_NEED (SEND)");
                    if (!file_send_list.empty()) {
                        if ( send_unit_need() == false) {
                            MSG_DBG_F("send_unit_need() failed");
                            printf("send file failed\n");
                            goto failed;
                        }
                        file_send_list.pop_front();
                    }
                    if (file_send_list.empty() && read_flag == 0) {
                        send_flag = 0;
                        if ( deliver(sockfd, socktag, DHT_DIR_SEND_END, nullptr, 0) == false) {
                            MSG_DBG_F("deliver failed");
                            goto failed;
                        }
                        MSG_DBG_F("SEND ALL SUCCESS, quit");
                        goto success;
                    }
                    break;

                }
                default: {
                    MSG_DBG("unknown stat, quit");
                    goto failed;
                }


            }
        }
        if (FD_ISSET(sockfd, &rset)) {
            deliver_header_t header;
            if ( deliver_get_header(sockfd, socktag, &header, &message_empty_flag) == false) {
                if (message_empty_flag == 1) {
                    MSG_DBG("Server exit");
                    goto failed;
                } else {
                    MSG_DBG_F("Socket error: %s", strerror(errno));
                    goto failed;
                }
            } else {
                // message_len = header.dh_data_len;

                message_len = ntohll(header.dh_data_len);
            }
            switch (process_indicator) {
                case PROCESS_DELIVER_INFO: {
                    MSG_DBG_F("DIR: Turn into PROCESS_HANDLE_INFO (READ)");
                    if ( header.dh_tag != DHT_DIR_INFORMATION) {
                        MSG_DBG("header mismatch");
                        goto failed;
                    }
                    if ( get_info_ack() == false) {
                        MSG_DBG_F("get_info_ack failed");
                        goto failed;
                    }


                    if (info_ack.di_stat == DI_NO_NEED_DELIVER) {
                        MSG_DBG("NO DEED DELIVER, quit");
                        printf("Dir is Latest.\n");
                        goto success;
                    } else if (info_ack.di_stat == DI_NEED_COMPARE) {
                        MSG_DBG("DI_NEED_COMPARE, start comparing");
                        printf("Dir need compare.\n");
                        process_indicator = PROCESS_DELIVER_TUPLES;
                        send_flag = 1, read_flag = 0;
                    } else if (info_ack.di_stat == DI_NEED_ALL) {
                        MSG_DBG("DI_NEED_ALL, start sending file");
                        printf("Traverse dir and send all files.\n");
                        process_indicator = PROCESS_DELIVER_FILES;
                        send_flag = 1, read_flag = 0;
                    } else {
                        MSG_DBG_F("Meet an error ack, quit.");
                        goto failed;
                    }
                    break;
                }

                case PROCESS_DELIVER_FILE_NEED: {
                    MSG_DBG_F("DIR: Turn into PROCESS_HANDLE_FILE_NEED (READ)");
                    if ( header.dh_tag != DHT_DIR_FILE_NEED) {
                        MSG_DBG("header mismatch");
                        goto failed;
                    }
                    if (message_len == 0) {
                        MSG_DBG_F("Message len is 0, should prepare to quit.");
                        read_flag = 0, send_flag = 1;
                    } else {
                        if ( recv_unit_need() == false) {
                            MSG_DBG("recv unit need failed");
                            goto failed;
                        }
                        read_flag = 1, send_flag = 1;
                    }
                    break;

                }
                default: {
                    MSG_DBG("unknown stat, quit");
                    goto failed;
                }
            }


        }
    }
success:
    MSG_DBG("Dir sync success");
    return true;
failed:
    MSG_DBG("Dir sync failed");
    return false;
}

bool Sync_dir_receiver::sync_start() {
    PROCESS_INDICATOR_T     process_indicator;
    int                     send_flag, read_flag;

    // === for select
    int                     maxfd = sockfd;
    fd_set                  rset, wset;
    int                     nready;
    process_indicator = PROCESS_DELIVER_INFO;
    send_flag = 0, read_flag = 1;
    bool                    dir_table_update_flag = true;

    while (1) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        if (process_indicator == PROCESS_DELIVER_INFO && send_flag == 1)
            FD_SET(sockfd, &wset);
        if (process_indicator == PROCESS_DELIVER_INFO && read_flag == 1)
            FD_SET(sockfd, &rset);
        if (process_indicator == PROCESS_DELIVER_FILES)
            FD_SET(sockfd, &rset);
        if (process_indicator == PROCESS_DELIVER_TUPLES)
            FD_SET(sockfd, &rset);
        if (process_indicator == PROCESS_DELIVER_FILE_NEED && send_flag == 1)
            FD_SET(sockfd, &wset);
        if (process_indicator == PROCESS_DELIVER_FILE_NEED && read_flag == 1)
            FD_SET(sockfd, &rset);

        MSG_DBG_F("Dir select wait");
        if ( (nready = select(maxfd+1, &rset, &wset, NULL, NULL)) == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                goto failed;
            }
        }
        if (FD_ISSET(sockfd, &rset)) {
            deliver_header_t header;
            if ( deliver_get_header(sockfd, socktag, &header, &message_empty_flag) == false) {
                if (message_empty_flag == 1) {
                    MSG_DBG("client exit");
                    goto failed;
                } else {
                    MSG_DBG_F("Socket error: %s", strerror(errno));
                    goto failed;
                }
            } else {
                // message_len = header.dh_data_len;

                message_len = ntohll(header.dh_data_len);
            }
            switch (process_indicator) {
                case PROCESS_DELIVER_INFO: {
                    MSG_DBG_F("DIR: Turn into PROCESS_HANDLE_INFO (READ)");
                    if ( header.dh_tag != DHT_DIR_INFORMATION) {
                        MSG_DBG("header mismatch");
                        goto failed;
                    }
                    MSG_DBG_F("message len is %d", message_len);

                    if ( recv_info() == false) {
                        MSG_DBG("recv_info failed");
                        goto failed;
                    }

                    read_flag = 0, send_flag = 1;
                    break;

                }
                case PROCESS_DELIVER_FILES: {
                    MSG_DBG_F("DIR: Turn into PROCESS_DELIVER_FILES (READ)");
                    file_tuple_t file_tuple;
                    if ( header.dh_tag == DHT_DIR_SEND_FILE) {
                        memset(&file_tuple, 0, sizeof(file_tuple));
                        if ( deliver_get_data(sockfd, socktag, &file_tuple, sizeof(file_tuple), &message_empty_flag) == false) {
                            MSG_DBG("deliver get data failed");
                            goto failed;
                        }
                        // file_tuples_t *     recv_file_tuples = (file_tuples_t*)calloc(1, sizeof(file_tuples_t)+sizeof(file_tuple_t));
                        // if ( deliver_get_data(sockfd, socktag, recv_file_tuples, sizeof(file_tuples_t)+sizeof(file_tuple_t), &message_empty_flag) == false) {
                        //     goto failed;
                        // }
                        if ( recv_unit() == false) {
                            MSG_DBG("recv_unit failed");
                            // dir_table_update_flag = false;
                            // continue other files
                        } else {
                            // file_table_controller.update_table(&file_tuple);
                        }
                    } else if (header.dh_tag == DHT_DIR_FINISH_SENDING) {
                        uint8_t sender_exit_code;
                        if ( deliver_get_data(sockfd, socktag, &sender_exit_code, sizeof(sender_exit_code), &message_empty_flag) == false) {
                            MSG_DBG("deliver get data failed");
                            goto failed;
                        }
                        if (sender_exit_code == 0x00) {
                            file_table_controller.update_table(&file_tuple);
                        } else {
                            dir_table_update_flag = false;
                            printf("Sender send file failed, do not update file table\n");
                        }

                    } else if( header.dh_tag == DHT_DIR_SEND_END) {
                        MSG_DBG("SEND END, quit");
                        goto success;
                    }
                    break;
                }
                case PROCESS_DELIVER_TUPLES: {
                    MSG_DBG("DIR: Turn into PROCESS_DELIVER_TUPLES (READ)");
                    if ( header.dh_tag != DHT_DIR_SEND_TUPLES) {
                        MSG_DBG("header mismatch");
                        goto failed;
                    }
                    if ( message_len == 0) {
                        process_indicator = PROCESS_DELIVER_FILE_NEED;
                        send_flag = 1, read_flag = 0;
                    } else {
                        if ( recv_unit_and_compare() == false) {
                            MSG_DBG("recv_unit_and_compare failed");
                            goto failed;
                        }
                    }
                    break;
                }
                case PROCESS_DELIVER_FILE_NEED: {
                    MSG_DBG("DIR: Turn into PROCESS_DELIVER_FILE_NEED (READ)");
                    file_tuple_t file_tuple;
                    if ( header.dh_tag == DHT_DIR_SEND_FILE) {
                        memset(&file_tuple, 0, sizeof(file_tuple));
                        if ( deliver_get_data(sockfd, socktag, &file_tuple, sizeof(file_tuple), &message_empty_flag) == false) {
                            MSG_DBG("deliver get data failed");
                            goto failed;
                        }
                        MSG_DBG_F("Receive a file's send request, the file is %s", file_tuple.filepath);

                        if ( recv_unit_need() == false) {
                            MSG_DBG_F("recv_unit_need of file '%s' failed, donot update table", file_tuple.filepath);
                            // dir_table_update_flag = false;
                            // goto failed;
                            // nothing, continue the other files send request
                        }
                        // } else {
                        //     if ( file_table_controller.update_table(&file_tuple) == false) {
                        //         MSG_DBG_F("Update table failed of file '%s'", file_tuple.filepath);
                        //     } else {
                        //         MSG_DBG_F("Update table success of file'%s'", file_tuple.filepath);
                        //     }
                        // }

                    } else if (header.dh_tag == DHT_DIR_FINISH_SENDING) {
                        uint8_t sender_exit_code;
                        if ( deliver_get_data(sockfd, socktag, &sender_exit_code, sizeof(sender_exit_code), &message_empty_flag) == false) {
                            MSG_DBG("deliver get data failed");
                            goto failed;
                        }
                        if (sender_exit_code == 0x00) {
                            file_table_controller.update_table(&file_tuple);
                        } else {
                            dir_table_update_flag = false;
                            printf("Sender send file failed, do not update file table\n");
                        }
                    } else if ( header.dh_tag == DHT_DIR_SEND_END) {
                        goto success;
                    }
                    break;
                }

            }

        }
        if (FD_ISSET(sockfd, &wset)) {
            switch (process_indicator) {
                case PROCESS_DELIVER_INFO: {
                    MSG_DBG_F("DIR: Turn into PROCESS_HANDLE_INFO (SEND)");
                    if ( send_info_ack() == false) {
                        MSG_DBG("send_info_ack failed");
                        goto failed;
                    }

                    if ( info_ack.di_stat == DI_NO_NEED_DELIVER) {
                        MSG_DBG("NO_NEED_DELIVER, quit");
                        goto success;
                    } else if ( info_ack.di_stat == DI_NEED_COMPARE) {
                        MSG_DBG("NEED COMPARE");
                        process_indicator = PROCESS_DELIVER_TUPLES;
                        read_flag = 1, send_flag = 0;
                    } else if (info_ack.di_stat == DI_NEED_ALL) {
                        MSG_DBG("NEED ALL");
                        process_indicator = PROCESS_DELIVER_FILES;
                        read_flag = 1, send_flag = 0;
                    } else {
                        MSG_DBG("FAILED, quit");
                        goto failed;
                    }
                    break;
                }
                case PROCESS_DELIVER_FILE_NEED: {
                    MSG_DBG("DIR: Turn into PROCESS_DELIVER_FILE_NEED (SEND)");
                    if ( !file_need_list.empty()) {
                        MSG_DBG_F("Send file need: %s", file_need_list.front().c_str());
                        send_unit_need();
                        file_need_list.pop_front();
                        read_flag = 1;
                    } else {
                        MSG_DBG_F("Send all files need");
                        send_flag = 0, read_flag = 1;
                        if ( deliver(sockfd, socktag, DHT_DIR_FILE_NEED, nullptr, 0) == false) {
                            MSG_DBG("deliver failed");
                            goto failed;
                        }
                    }
                    break;
                }
                default: {
                    MSG_DBG("unknown stat, quit");
                    goto failed;
                }

            }
        }
    }
success:
    if (dir_table_update_flag == true) {
        MSG_DBG_F("recv all success, then update dir table");
        dir_table_controller.update_table(dir_tuple);
    } else {
        printf("One or more files recv failed, do not update dir table\n");
    }
    if (dir_tuple != nullptr) Dir_table_controller::free_tuples(dir_tuple);
    MSG_DBG("Dir sync success");
    return true;
failed:
    if (dir_tuple != nullptr) Dir_table_controller::free_tuples(dir_tuple);
    MSG_DBG("Dir sync failed");
    return false;

}

// typedef struct {
//     char    send_dir_hash[64];
//     char    dest_address[255];
// } info_t;

bool Sync_dir_sender::send_info() {

    dir_tuple_t * dir_tuple = nullptr;
    if ( (dir_tuple = (dir_tuple_t *)dir_table_controller.get_single_tuple(dirpath_from_database)) != nullptr) {
        printf("Synchronize a folder: '%s'\n", dir_tuple->dirpath);
        char table_name[100];
        memset(table_name, 0, sizeof(table_name));
        snprintf(table_name, sizeof(table_name)-1, "table_%s", dir_tuple->dirpath_hash);

        file_table_controller.bind(db_path, table_name);

        dir_tuple->modify_time = htonll(dir_tuple->modify_time);
        dir_tuple->update_flag = htons(dir_tuple->update_flag);
        dir_tuple->deleted_flag = htons(dir_tuple->deleted_flag);
        if ( deliver(sockfd, socktag, DHT_DIR_INFORMATION, dir_tuple, sizeof(dir_tuple_t)) == false) {
            MSG_DBG("deliver filed");
            goto failed;
        }
        basepath_len = strlen(dir_tuple->dirpath);
        memset(basepath_hash, 0, sizeof(basepath_hash));
        strncpy(basepath_hash, dir_tuple->dirpath_hash, sizeof(basepath_hash));

    } else {
        printf("Your dir '%s' has no file to send.\n", dir_tuple->dirpath);
        MSG_DBG("No data to send");
        goto failed;
    }

    if (dir_tuple != nullptr) Dir_table_controller::free_tuples(dir_tuple);
    return true;
failed:
    if (dir_tuple != nullptr) Dir_table_controller::free_tuples(dir_tuple);
    return false;
}

bool Sync_dir_receiver::recv_info() {
    dir_tuple_t *   dir_recv_tuple = nullptr;
    // int              set_flag = 0;


    if ( (dir_recv_tuple = (dir_tuple_t *)calloc(1, sizeof(dir_tuple_t))) == nullptr) {
        MSG_DBG_F("Memory allocate failed: %s", strerror(errno));
        goto failed;
    }
    if ( deliver_get_data(sockfd, socktag, dir_recv_tuple, sizeof(dir_tuple_t), &message_empty_flag) == false) {
        MSG_DBG_F("deliver_get_data failed");
        goto failed;
    }

    char table_name[100];
    memset(table_name, 0, sizeof(table_name));
    snprintf(table_name, sizeof(table_name)-1, "table_%s", dir_recv_tuple->dirpath_hash);


//     // /path/to/your/recv/xxxxx(sha256)
//     memset(cur_recv_dirpath, 0, sizeof(cur_recv_dirpath));
//     snprintf(cur_recv_dirpath, sizeof(cur_recv_dirpath)-1, "%s/%s", recv_rootpath, dir_recv_tuples->tuples[0].dirpath_hash);
//
//     // /path/to/your/verify/xxxxxx(sha256)
//     memset(cur_verify_dirpath, 0, sizeof(cur_verify_dirpath));
//     snprintf(cur_verify_dirpath, sizeof(cur_verify_dirpath)-1, "%s/%s", verify_rootpath, dir_recv_tuples->tuples[0].dirpath_hash);

    if ( file_table_controller.bind(nullptr, table_name) == false) {
        MSG_DBG_F("file_table_controller.bind() failed");
        goto failed;
    }

    // dir_recv_tuple->size = ntohl(dir_recv_tuple->size);
    // MSG_DBG_F("size = %d, filepath = %s", dir_recv_tuple->size, dir_recv_tuple->dirpath);

    // if ( dir_recv_tuple->size != 1) {
    //     info_ack.di_stat = DI_FAILED;
    //     MSG_DBG_F("dir_recv_tuples->size = %d, not expect", dir_recv_tuple->size);
    // } else {
    dir_recv_tuple->modify_time = ntohll(dir_recv_tuple->modify_time);

    dir_tuple_t *    dir_local_tuple;
    if ( (dir_local_tuple = (dir_tuple_t *)dir_table_controller.get_single_tuple(dir_recv_tuple->dirpath)) != nullptr) {
        if (dir_local_tuple->modify_time == dir_recv_tuple->modify_time) {
            MSG_DBG_F("DIR ('%s'): modify time match, no need deliver", dir_local_tuple->dirpath);
            info_ack.di_stat = DI_NO_NEED_DELIVER;
            printf("Deal dir: '%s', server has the lastest, don't need to send\n", dir_recv_tuple->dirpath);
        } else {
            MSG_DBG_F("DIR ('%s'): modify time mismatch, need to compare", dir_local_tuple->dirpath);
            MSG_DBG_F("dir modify = %ld", dir_local_tuple->modify_time);
            info_ack.di_stat = DI_NEED_COMPARE;
            printf("Deal dir: '%s', server need to compare it with client\n", dir_recv_tuple->dirpath);
        }
    } else {
        MSG_DBG_F("DIR ('%s'): not exist in local database, need all", dir_local_tuple->dirpath);
        info_ack.di_stat = DI_NEED_ALL;
        printf("Deal dir: '%s', server dose not have it, read all files of it\n", dir_recv_tuple->dirpath);
    }

    if (dir_local_tuple != nullptr) {
        Dir_table_controller::free_tuples(dir_local_tuple);
    }
    // }

    // if (dir_recv_tuples != nullptr) {
    //     free(dir_recv_tuples);
    // }
    dir_tuple = dir_recv_tuple;
    return true;
failed:
    if (dir_recv_tuple != nullptr) {
        free(dir_recv_tuple);
    }
    dir_tuple = nullptr;
    return false;
}

bool Sync_dir_receiver::send_info_ack() {
    // info_ack.di_stat = htonl(info_ack.di_stat);
    if ( deliver(sockfd, socktag, DHT_DIR_INFORMATION, &info_ack, sizeof(info_ack)) == false) {
        MSG_DBG("Deliver failed");
        return false;
    }
    return true;
}


bool Sync_dir_sender::get_info_ack() {
    if ( deliver_get_data(sockfd, socktag, &info_ack, sizeof(dir_info_ack_t), &message_empty_flag) == false) {
        MSG_DBG("deliver_get_data() failed");
        return false;
    }
    return true;
}

bool Sync_dir_sender::send_unit() {
    file_tuples_t *     file_tuples = nullptr;
    int                 offset = 0;


    while ( (file_tuples = (file_tuples_t *)file_table_controller.get_tuples(1, offset++)) != nullptr) {
        MSG_DBG_F("Send file '%s'", file_tuples->tuples[0].filepath);
        // printf("Sending file '%s'\n", file_tuples->tuples[0].filepath);
        // everify time send a DHT_DIR_SEND_FILE header with the tuple of the file, and then send_single_file
        if ( file_tuples->tuples[0].deleted_flag != 1) {

            if ( deliver(sockfd, socktag, DHT_DIR_SEND_FILE, &file_tuples->tuples[0], sizeof(file_tuple_t)) == false) {
                MSG_DBG_F("deliver failed");
                return false;
            }

            uint8_t process_exit_code = 0x00;
            if ( send_single_file_process(deliver_file_port, socktag, file_tuples->tuples[0].filepath, basepath_len, basepath_hash, verify_rootpath) == false) {
                // MSG_DBG_F("send_single_file_process() of file '%s' failed", file_tuples->tuples[0].filepath);
                printf("Send file '%s' failed.\n", file_tuples->tuples[0].filepath);
                process_exit_code = 0x01;
                goto failed;
            } else {
//                 // wait the send_file_process
//                 int status;
//                 waitpid(child_pid, &status, 0);
//
//                 if (WIFEXITED(status)) {
//                     MSG_DBG_F("Process exit with stat: %d", WEXITSTATUS(status));
//                     int exit_code = WEXITSTATUS(status);
//                     if (exit_code == EXIT_SUCCESS) {
//                         MSG_DBG_F("Process return EXIT_SUCCESS");
//                     } else {
//                         MSG_DBG_F("Process return EXIT_FAILURE");
//                         success_flag = false;
//                     }
//                 } else if (WIFSIGNALED(status)) {
//                     MSG_DBG_F("Process is killed : %d", WTERMSIG(status));
//                     success_flag = false;
//                 }

            }

            if ( deliver(sockfd, socktag, DHT_DIR_FINISH_SENDING, &process_exit_code, sizeof(process_exit_code)) == false) {
                File_table_controller::free_tuples(file_tuples);
                goto failed;
            }

            // if ( send_single_file(file_sockfd, socktag, file_tuples->tuples[0].filepath, basepath_len, basepath_hash, verify_rootpath) == false) {
            //     // File_table_controller::free_tuples(file_tuples);
            //     // MSG_DBG_F("send_single_file failed");
            //     printf("Send file '%s' failed.\n", file_tuples->tuples[0].filepath);
            //     // return false;
            // }
        }
        // printf("Send file '%s' success.\n", file_tuples->tuples[0].filepath);
        File_table_controller::free_tuples(file_tuples);
    }

    // no file to send
    if (offset == 0) {
        MSG_DBG_F("No file to send");
        // return false;
    }

    // send DHT_DIR_SEND_END will make server goto success
    if ( deliver(sockfd, socktag, DHT_DIR_SEND_END, NULL, 0) == false)
        return false;
    return true;
failed:
    // if ( deliver(sockfd, socktag, DHT_DIR_SEND_END, NULL, 0) == false)
    //     return false;
    return false;
}

bool Sync_dir_receiver::recv_unit() {
    // MSG_DBG_F("recv_root_path = %s, cur_verify_dirpath is %s",recv_rootpath, cur_verify_dirpath);

//     pid_t child_pid;
//     // char  ip_address_str[20];
//
//
//     // TODO: wait the files
//     // MSG_DBG_F("local ip_address_str is %s", )
//     if ( recv_single_file_process(process_attr, socktag, recv_rootpath, verify_rootpath, &child_pid) == false) {
//         return false;
//     } else {
//         int status;
//         waitpid(child_pid, &status, 0);
//
//         if (WIFEXITED(status)) {
//             MSG_DBG_F("Process exit with stat: %d", WEXITSTATUS(status));
//             int exit_code = WEXITSTATUS(status);
//             if (exit_code == EXIT_SUCCESS) {
//                 MSG_DBG_F("Process return EXIT_SUCCESS");
//             } else {
//                 MSG_DBG_F("Process return EXIT_FAILURE");
//                 return false;
//             }
//         } else if (WIFSIGNALED(status)) {
//             MSG_DBG_F("Process is killed : %d", WTERMSIG(status));
//             // return false;
//             return false;
//         }
//     }
    return true;
}

bool Sync_dir_sender::send_unit_entrys() {
    file_tuples_t *     file_tuples = nullptr;
    int                 offset = 0;
    std::stack<file_tuples_t *> file_tuples_to_free;

    while ( (file_tuples = (file_tuples_t *)file_table_controller.get_tuples(10, offset)) != nullptr) {
        MSG_DBG_F("file_tuples->size = %ld", file_tuples->size);
        for (int i = 0; i < file_tuples->size; i++) {
            file_tuples->tuples[i].modify_time = htonll(file_tuples->tuples[i].modify_time);
            file_tuples->tuples[i].file_size = htonll(file_tuples->tuples[i].file_size);
            file_tuples->tuples[i].update_flag = htons(file_tuples->tuples[i].update_flag);
            file_tuples->tuples[i].is_file_or_dir = htons(file_tuples->tuples[i].is_file_or_dir);
            file_tuples->tuples[i].deleted_flag = htons(file_tuples->tuples[i].deleted_flag);
        }
        int data_len = sizeof(file_tuples_t)+ file_tuples->size*sizeof(file_tuple_t);
        MSG_DBG_F("data_len = %d", data_len);
        if ( deliver(sockfd, socktag, DHT_DIR_SEND_TUPLES, file_tuples, data_len) == false) {
            File_table_controller::free_tuples(file_tuples);
            return false;
        }
        offset += file_tuples->size;
        file_tuples_to_free.push(file_tuples);
    }
    while (!file_tuples_to_free.empty()) {
        File_table_controller::free_tuples(file_tuples_to_free.top());
        file_tuples_to_free.pop();
    }

    if ( deliver(sockfd, socktag, DHT_DIR_SEND_TUPLES, NULL, 0) == false)
        return false;

    return true;
}

bool Sync_dir_receiver::recv_unit_and_compare() {
    file_tuples_t *      file_recv_tuples = nullptr;
    file_tuple_t *       file_local_tuple = nullptr;

    MSG_DBG_F("message_len is %d", message_len);
    if ( (file_recv_tuples = (file_tuples_t *)calloc(1, message_len)) == nullptr) {
        MSG_DBG("Memory allocate failed");
        goto failed;
    }
    if ( deliver_get_data(sockfd, socktag, file_recv_tuples, message_len, &message_empty_flag) == false) {
        MSG_DBG("Recv data failed");
        goto failed;
    }
    MSG_DBG_F("file_recv_tuples->size = %d", file_recv_tuples->size);
    printf("Need to compare %d files\n", file_recv_tuples->size);

    for (int i = 0; i < file_recv_tuples->size; i++) {
        MSG_DBG_F("i = %d", i);
        file_recv_tuples->tuples[i].modify_time = ntohll(file_recv_tuples->tuples[i].modify_time);
        file_recv_tuples->tuples[i].file_size = ntohll(file_recv_tuples->tuples[i].file_size);
        file_recv_tuples->tuples[i].update_flag = ntohs(file_recv_tuples->tuples[i].update_flag);
        file_recv_tuples->tuples[i].is_file_or_dir = ntohs(file_recv_tuples->tuples[i].is_file_or_dir);
        file_recv_tuples->tuples[i].deleted_flag = ntohs(file_recv_tuples->tuples[i].deleted_flag);

        if ( (file_local_tuple = (file_tuple_t*)file_table_controller.get_single_tuple(file_recv_tuples->tuples[i].filepath)) == nullptr) {
            // TODO: 应在接收文件成功后修改table
            // file_need_list.push_back(file_recv_tuples->tuples[i].filepath);
            // dir_table_controller.update_table(&(file_recv_tuples->tuples[i]));
            // file_tuple_t tuple_node;
            // memcpy(&tuple_node, &(file_recv_tuples->tuples[i]), sizeof(tuple_node));
            file_need_list.push_back(file_recv_tuples->tuples[i].filepath);

            MSG_DBG_F("Local database doesn't have file '%s', need to recv it", file_recv_tuples->tuples[i].filepath);
            printf("Local database doesn't have file '%s', need to recv it\n", file_recv_tuples->tuples[i].filepath);
        } else {
            if ( file_recv_tuples->tuples[i].modify_time == file_local_tuple->modify_time &&
                file_recv_tuples->tuples[i].file_size == file_local_tuple->file_size) {
                // do nothing
                MSG_DBG_F("Local database has file '%s', modify_time and file_size of it is match, donot need to recv it", file_local_tuple->filepath);
                printf("Local database has file '%s', modify_time and file_size of it is match, donot need to recv it\n", file_local_tuple->filepath);
            } else {
                // file_tuple_t tuple_node;
                // memcpy(&tuple_node, &(file_recv_tuples->tuples[i]), sizeof(tuple_node));
                file_need_list.push_back(file_recv_tuples->tuples[i].filepath);
                // dir_table_controller.update_table(&(file_recv_tuples->tuples[i]));

                MSG_DBG_F("Local database has file '%s', but need to compare it", file_local_tuple->filepath);
                printf("Local database has file '%s', but need to compare it", file_local_tuple->filepath);
            }
            File_table_controller::free_tuples(file_local_tuple);
        }
    }

    if ( file_recv_tuples != nullptr)
        free(file_recv_tuples);
    return true;
failed:
    if ( file_recv_tuples != nullptr)
        free(file_recv_tuples);
    return false;

}


bool Sync_dir_receiver::send_unit_need() {
    if (!file_need_list.empty()) {
        if ( deliver(sockfd, socktag, DHT_DIR_FILE_NEED, file_need_list.front().c_str(), file_need_list.front().size()) == false)
            return false;
        // file_need_list.pop_front();
    }

    return true;
}

bool Sync_dir_sender::recv_unit_need() {
    char    filepath[4096];
    memset(filepath, 0, sizeof(filepath));
    if ( deliver_get_data(sockfd, socktag, filepath, message_len, &message_empty_flag) == false) {
        MSG_DBG("DIR: get data failed");
        return false;
    }
    file_send_list.push_front(filepath);
    MSG_DBG_F("Recv a need node: file '%s'", filepath);
    return true;
}


// send a DHT_DIR_SEND_FILE header, with a tuple of the file; if receiver recv file success, he will update his local database
bool Sync_dir_sender::send_unit_need() {
    file_tuple_t * file_tuple;

    if ( (file_tuple = (file_tuple_t *)file_table_controller.get_single_tuple(file_send_list.front().c_str())) == nullptr) {
        MSG_DBG_F("get single tuple of file '%s' failed", file_send_list.front().c_str());
        return false;
    }
    if (!file_send_list.empty()) {
        if ( deliver(sockfd, socktag, DHT_DIR_SEND_FILE, file_tuple, sizeof(file_tuple_t)) == false) {
            MSG_DBG_F("deliver failed");
            return false;
        }
        // if ( deliver(sockfd, socktag, DHT_DIR_SEND_FILE, &file_tuple, sizeof(file_tuple_t)) == false) {
        //     MSG_DBG_F("deliver failed");
        //     return false;
        // }


        uint8_t process_exit_code = 0x00;
        if ( send_single_file_process(deliver_file_port, socktag, file_tuple->filepath, basepath_len, basepath_hash, verify_rootpath) == false) {
            MSG_DBG_F("send_single_file_process() of file '%s' failed", file_tuple->filepath);
            printf("Send file '%s' failed.\n", file_tuple->filepath);
            process_exit_code = 0x01;
            return false;
        } else {
//             // wait the send_file_process
//             int status;
//             waitpid(child_pid, &status, 0);
//
//             if (WIFEXITED(status)) {
//                 MSG_DBG_F("Process exit with stat: %d", WEXITSTATUS(status));
//                 int exit_code = WEXITSTATUS(status);
//                 if (exit_code == EXIT_SUCCESS) {
//                     MSG_DBG_F("Process return EXIT_SUCCESS");
//                 } else {
//                     MSG_DBG_F("Process return EXIT_FAILURE");
//                     return false;
//                 }
//             } else if (WIFSIGNALED(status)) {
//                 MSG_DBG_F("Process is killed : %d", WTERMSIG(status));
//                 return false;
//             }
        }

        if ( deliver(sockfd, socktag, DHT_DIR_FINISH_SENDING, &process_exit_code, sizeof(process_exit_code)) == false) {
            MSG_DBG_F("deliver failed");
            return false;
        }

        // if ( send_single_file(file_sockfd, socktag, file_tuples->tuples[0].filepath, basepath_len, basepath_hash, verify_rootpath) == false) {
        //     // File_table_controller::free_tuples(file_tuples);
        //     // MSG_DBG_F("send_single_file failed");
        //     printf("Send file '%s' failed.\n", file_tuples->tuples[0].filepath);
        //     // return false;
        // }
        // send_single_file(file_sockfd, socktag, file_send_list.front().c_str(), basepath_len, basepath_hash, verify_rootpath);
    }
    return true;
}

bool Sync_dir_receiver::recv_unit_need() {
    // if ( recv_single_file(file_sockfd, socktag, recv_rootpath, verify_rootpath) == false) {
    //     MSG_DBG_F("recv file filed");
    //     return false;
    // }
//     pid_t child_pid;
//     // char  ip_address_str[20];
//
//
//     // TODO: wait the files
//     // MSG_DBG_F("local ip_address_str is %s", )
//     if ( recv_single_file_process(process_attr, socktag, recv_rootpath, verify_rootpath, &child_pid) == false) {
//         return false;
//     } else {
//         int status;
//         waitpid(child_pid, &status, 0);
//         if (WIFEXITED(status)) {
//             MSG_DBG_F("Process exit with stat: %d", WEXITSTATUS(status));
//             int exit_code = WEXITSTATUS(status);
//             if (exit_code == EXIT_SUCCESS) {
//                 MSG_DBG_F("Process return EXIT_SUCCESS");
//             } else {
//                 MSG_DBG_F("Process return EXIT_FAILURE");
//                 return false;
//             }
//
//         } else if (WIFSIGNALED(status)) {
//             MSG_DBG_F("Process is killed : %d", WTERMSIG(status));
//             // return false;
//             return false;
//         }
//     }
    return true;
}
