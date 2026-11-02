#include "core/dir_sync.hpp"
#include "core/table_control.hpp"
#include "deliver/deliver.h"
#include "common/check_error.hpp"
#include "common/msg_print.h"
#include "config/get_config.hpp"
#include "config/get_config.hpp"
#include "utils/performance_test.hpp"
#include "common/deleter.hpp"
#include "yaml-cpp/exceptions.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <forward_list>
#include <memory>
#include <netdb.h>
#include <pthread.h>
#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <spawn.h>


// static std::forward_list<std::string>   dir_send_list;
// static pthread_mutex_t                  dir_send_list_mutex;
// static pthread_cond_t                   dir_send_list_cond;

static struct dir_send_list {
    std::forward_list<std::string>      dirpath_list;
    pthread_mutex_t                     dirpath_list_mutex;
    // pthread_cond_t                      dirpath_list_cond;
    pthread_t                           thread_list[4];
    // int                                 traverse_done_flag;
    // pthread_cond_t              thread_wait_cond;
} ds_list;


static posix_spawnattr_t                process_attr;

static posix_spawn_file_actions_t       process_action; // Redirect the send_single_file process's stdout to the current process's stdout/stderror

extern char ** environ;

bool thread_make(int no);
int main(int argc, char * argv[])
{
    Dir_table_controller    dir_table_controller;
    char                    verify_dir[4096];
    char                    db_dir[4096];

    memset(verify_dir, 0, sizeof(verify_dir));
    get_config_verifypath(verify_dir, sizeof(verify_dir));
    memset(db_dir, 0, sizeof(db_dir));
    snprintf(db_dir, sizeof(db_dir), "%s/%s", verify_dir, "lurk_database.db");

    // for test
    // memset(db_dir, 0, sizeof(db_dir));
    // snprintf(db_dir, sizeof(db_dir), "%s/%s", "./send/verify", "lurk_database.db");

    if ( dir_table_controller.bind(db_dir, "Sender_map_table") == false) {
        MSG_DBG_F("dir_table_ctrl.bind() failed");
        printf("Database error, your verify_path is '%s', your database path is '%s'\n", verify_dir, db_dir);
        return EXIT_FAILURE;
    }

    pthread_mutex_init(&ds_list.dirpath_list_mutex, NULL);

    posix_spawnattr_init(&process_attr);
    posix_spawnattr_setflags(&process_attr, POSIX_SPAWN_SETPGROUP);
    posix_spawnattr_setpgroup(&process_attr, getpid());

    // posix_spawnattr_t               process_attr; // enable the process join a same process group, it is convenient to kill all of them
    posix_spawn_file_actions_init(&process_action);
    // posix_spawnattr_init(&process_attr);
    // posix_spawnattr_setflags(&process_attr, POSIX_SPAWN_SETPGROUP);
    // posix_spawnattr_setpgroup(&process_attr, getpid());

    posix_spawn_file_actions_adddup2(&process_action, STDOUT_FILENO, STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&process_action, STDERR_FILENO, STDERR_FILENO);


    dir_tuples_t *          dir_tuples;
    int offset = 0;
    while (1) {
        if ( (dir_tuples = (dir_tuples_t *)dir_table_controller.get_update_tuples(100, offset)) != nullptr) {
            for (int i = 0; i < dir_tuples->size; i++) {
                ds_list.dirpath_list.push_front(std::string(dir_tuples->tuples[i].dirpath));
            }
        } else {
            MSG_DBG_F("Read dir table empty, quit");
            break;
        }
        MSG_DBG_F("Read %u dirs", dir_tuples->size);
        offset += dir_tuples->size;
        if (dir_tuples->size != 100) {
            break;
        }
    }
    if (offset == 0) {
        MSG_DBG_F("No record dir in Sender_map_table, quit");
        printf("No record dir in database\n");
        return EXIT_SUCCESS;
    }

    for (int i = 0; i < 4; i++) {
        ds_list.thread_list[i] = 0;
    }

    if (offset < 4) {
        for (int i = 0; i < offset; i++) {
            if ( thread_make(i) == false)
                break;
        }
    } else {
        for (int i = 0; i < 4; i++) {
            if (thread_make(i) == false) {
                break;
            }
        }
    }

    // pthread_mutex_lock(&ds_list.dirpath_list_mutex);
    // while (!ds_list.dirpath_list.empty()) {
    //     pthread_cond_wait(&ds_list.thread_wait_cond, &ds_list.dirpath_list_mutex);
    // }
    // pthread_mutex_unlock(&ds_list.dirpath_list_mutex);

    for (int i = 0; i < 4; i++) {
        if (ds_list.thread_list[i] != 0)
            pthread_join(ds_list.thread_list[i], nullptr);
    }

    pthread_mutex_destroy(&ds_list.dirpath_list_mutex);

    posix_spawnattr_destroy(&process_attr);
    posix_spawn_file_actions_destroy(&process_action);


    printf("[lurk client] exit\n");
    return EXIT_SUCCESS;
}

typedef struct {
    int         thread_no;
    sock_tag_t  socktag;
} dir_thread_arg_t;


bool
thread_make(int no)
{
    void * dir_sync_thread(void * thread_no);

    int pthread_rtn;
    dir_thread_arg_t * dir_thread_arg = new dir_thread_arg_t;
    dir_thread_arg->thread_no = no;
    dir_thread_arg->socktag = ST_UNIX;
    if ( (pthread_rtn = pthread_create(&ds_list.thread_list[no], NULL, &dir_sync_thread, dir_thread_arg)) != 0) {
        MSG_DBG_F("Pthread create error: %s", strerror(errno));
        return false;
    }
    MSG_DBG_F("Make thread %d", no);
    return true;
}


void *
dir_sync_thread(void * arg)
{
    dir_thread_arg_t *  dir_thread_arg = (dir_thread_arg_t *)arg;
try {
    while (1) {
        pthread_mutex_lock(&ds_list.dirpath_list_mutex);

        if (ds_list.dirpath_list.empty()) {
            MSG_DBG_F("dir_sync_thread%d exit", dir_thread_arg->thread_no);
            delete dir_thread_arg;
            pthread_mutex_unlock(&ds_list.dirpath_list_mutex);
            pthread_exit(nullptr);
        }

        auto node = ds_list.dirpath_list.front();
        ds_list.dirpath_list.pop_front();

        pthread_mutex_unlock(&ds_list.dirpath_list_mutex);

        printf("[lurk client] dir '%s' sync thread start\n", node.c_str());

        // first update
        char * argv[3];
        argv[0] = (char *)"lurk_update_dir";
        argv[1] = (char *)node.c_str();
        argv[2] = NULL;
        pid_t pid_rtn;
        int spawn_rtn;
        if ( (spawn_rtn = posix_spawn(&pid_rtn, "./lurk_update_dir", &process_action, &process_attr, argv, environ)) != 0) {
            // MSG_DBG_F("create process errro : %s", strerror(errno));
            printf("[lurk client] posix_spawn: %s\n", strerror(spawn_rtn));
            continue; // no update, no send
        }
        MSG_DBG_F("create process success, pid = %d", pid_rtn);

        int status;
        waitpid(pid_rtn, &status, 0);
        if (WIFEXITED(status)) {
            MSG_DBG_F("Process exit with stat: %d", WEXITSTATUS(status));
            int exit_code = WEXITSTATUS(status);
            if (exit_code == EXIT_SUCCESS) {
                MSG_DBG_F("Process return EXIT_SUCCESS");
            } else {
                MSG_DBG_F("Process return EXIT_FAILURE");
                continue;
            }
        } else if (WIFSIGNALED(status)) {
            MSG_DBG_F("Process is killed : %d", WTERMSIG(status));
            continue;
        }



        std::unique_ptr<int, fd_deleter>    sockfdp(new int(0), fd_deleter());
        std::shared_ptr<int>                sync_dir_socketp(new int(0), fd_deleter());
        // int                                 sync_dir_socket;
        char                                server_ip_str[NI_MAXHOST];
        uint16_t                            server_port;
        struct sockaddr_in                  servaddr;
        // read it as big endian
        uint16_t                deliver_port;
        char                    verify_path[4096];
        
        // now return immediately
        check_syscall_error((*sockfdp.get() = socket(AF_INET, SOCK_STREAM, 0)), "socket");

        memset(server_ip_str, 0, sizeof(server_ip_str));
        get_config_server_ip(server_ip_str, sizeof(server_ip_str));

        server_port = get_config_server_port();

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(server_port);

        check_syscall_error(inet_pton(AF_INET, server_ip_str, &servaddr.sin_addr), "inet_pton");
        check_syscall_error(connect(*sockfdp.get(), (struct sockaddr *)&servaddr, sizeof(servaddr)), "first connect");

        // for future use
        uint8_t     message = 0x01;
        check_syscall_error(send(*sockfdp.get(), &message, sizeof(message), 0), "send");
        check_syscall_error(recv(*sockfdp.get(), &deliver_port, sizeof(deliver_port), 0), "recv");

        // close sockfd
        sockfdp.reset();

        // New connection success
        printf("Find listening server, via server ip : %s, server port : %d, \n", server_ip_str, ntohs(deliver_port));

        check_syscall_error((*sync_dir_socketp.get() = socket(AF_INET, SOCK_STREAM, 0)), "socket");
        struct sockaddr_in new_servaddr;
        memcpy(&new_servaddr, &servaddr, sizeof(struct sockaddr_in));
        new_servaddr.sin_port = deliver_port;
        check_syscall_error(connect(*sync_dir_socketp.get(), (struct sockaddr *)&new_servaddr, sizeof(new_servaddr)), "second connect");
        memset(verify_path, 0, sizeof(verify_path));
        get_config_verifypath(verify_path, sizeof(verify_path));


        uint16_t    deliver_file_port;
        // thread_no is begin with 0
        deliver_file_port = get_config_deliver_file_port(dir_thread_arg->thread_no);

        printf("[lurk_client] server address = (%s :: %hu), begin sync dir: '%s'", server_ip_str, deliver_port, node.c_str());

        Performance_test    pf_test;
        pf_test.begin();
        Sync_dir_sender     sender(sync_dir_socket, deliver_file_port, ST_COMMON,  node.c_str(), verify_path, getpid());
        bool                sync_success_flag = true;
        pf_test.end();

        if ( sender.sync_start() == false) {
            sync_success_flag = false;
        }
        // shutdown(*sync_dir_socketp.get(), SHUT_RDWR);
        // close(sync_dir_socket);

        if (sync_success_flag == false) {
            printf("[lurk client] 🥲sync dir '%s' meet some failures\n", node.c_str());
            continue;
        } else {
            printf("[lurk client] 😀Sync dir %s success, costs %f cpu seconds, %f actual seconds\n", node.c_str(), pf_test.get_times().cpu_duration, pf_test.get_times().time_duration);
            continue;
        }
    }
} catch (YAML::Exception & e) {
    std::cerr << "[lurk client] sync dir catch config file error => " << e.what() << std::endl;
} catch (std::runtime_error & e) {
    std::cerr << "[lurk client] sync dir catch runtime error => " << e.what() << std::endl;
} catch (std::exception & e) {
    std::cerr << "[lurk client] sync dir catch other error => " << e.what() << std::endl;
}

    delete dir_thread_arg;
    pthread_exit(nullptr);
}
