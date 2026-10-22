#include "core/dir_sync.hpp"
#include "core/table_control.hpp"
#include "deliver/deliver.h"
#include "sys/msg_print.h"
#include "tools/get_config.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstring>
#include <forward_list>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" {
    #include <sys/socket.h>
    #include <sys/uio.h>
    #include <arpa/inet.h>
}

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
    if ( get_config_verifypath_client(verify_dir) == false) {
        MSG_DBG_F("verify path get failed");
        return EXIT_FAILURE;
    }
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

bool
thread_make(int no)
{
    void * dir_sync_thread(void * thread_no);

    int pthread_rtn;
    int * arg = new int;
    *arg = no;
    if ( (pthread_rtn = pthread_create(&ds_list.thread_list[no], NULL, &dir_sync_thread, arg)) != 0) {
        MSG_DBG_F("Pthread create error: %s", strerror(errno));
        return false;
    }
    MSG_DBG_F("Make thread %d", no);
    return true;
}


void *
dir_sync_thread(void * thread_no)
{
    while (1) {
        pthread_mutex_lock(&ds_list.dirpath_list_mutex);

        if (ds_list.dirpath_list.empty()) {
            MSG_DBG_F("dir_sync_thread%d exit", *(int*)thread_no);
            delete (int *)thread_no;
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



        int                     sockfd;
        int                     sync_dir_socket;
        char                    server_ip_str[NI_MAXHOST];
        uint16_t                server_port;
        struct sockaddr_in      servaddr;
        // read it as big endian
        uint16_t                deliver_port;
        char                    verify_path[4096];
        // int *                   thread_rtn = (int*)calloc(1, sizeof(int));


        if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            // MSG_DBG_F("Socket create failed: %s", strerror(errno));
            perror("[lurk client] socket");
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }

        memset(server_ip_str, 0, sizeof(server_ip_str));
        if ( get_config_server_ip_client(server_ip_str) == false) {
            printf("Cannot read server_ip in config, check your server_ip field in config file\n");
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }

        server_port = get_config_server_port_client();

        if ( server_port == 0) {
            printf("Cannot read server_port in config, check your server_port field in config file\n");
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(server_port);
        if (inet_pton(AF_INET, server_ip_str, &servaddr.sin_addr) != 1) {
            // printf("Your format of server_ip in config file is not correct\n");
            perror("[lurk client] inet_pton");
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }


        if ( (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
            // MSG_DBG("connect failed");
            perror("[lurk client] first connect");
            close(sockfd);
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }

        uint8_t     message = 0x01;
        if ( send(sockfd, &message, sizeof(message), 0) == -1) {
            // MSG_DBG_F("Send failed: %s", strerror(errno));
            perror("[lurk client] send");
            close(sockfd);
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }


        if ( recv(sockfd, &deliver_port, sizeof(deliver_port), 0) == -1) {
            // MSG_DBG_F("Recv failed: %s", strerror(errno));
            perror("[lurk client] recv");
            close(sockfd);
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }
        MSG_DBG_F("deliver_port = %d", ntohs(deliver_port));

        close(sockfd);

        // New connection success
        printf("Find listening server, via server ip : %s, server port : %d, \n", server_ip_str, ntohs(deliver_port));
        if ( (sync_dir_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            MSG_DBG_F("Socket create failed: %s", strerror(errno));
            perror("[lurk client] socket");
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }

        struct sockaddr_in new_servaddr;
        memcpy(&new_servaddr, &servaddr, sizeof(struct sockaddr_in));
        new_servaddr.sin_port = deliver_port;

        // sleep(5);
        if ( (connect(sync_dir_socket, (struct sockaddr *)&new_servaddr, sizeof(new_servaddr))) == -1) {
            // MSG_DBG_F("Connect error : %s", strerror(errno));
            perror("[lurk client] second connect");
            close(sync_dir_socket);
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }
        // printf("Begin delivering\n");

        memset(verify_path, 0, sizeof(verify_path));
        if ( get_config_verifypath_client(verify_path) == false) {
            MSG_DBG_F("Read verify path failed");
            close(sync_dir_socket);
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        }

        char                    verify_dir[4096];
        memset(verify_dir, 0, sizeof(verify_dir));
        get_config_verifypath_client(verify_dir);

        uint16_t    deliver_file_port;
        // thread_no is begin with 0
        if ( (deliver_file_port = get_config_deliver_file_port((*(int*)thread_no)+1)) == false) {
            MSG_DBG_F("Read config_deliver_file_port failed");
            close(sync_dir_socket);
            continue;
        }
        MSG_DBG_F("Server_ip = %s, deliver_file_port = %hu, begin sync dir: '%s'", server_ip_str, deliver_port, node.c_str());

        Sync_dir_sender     sender(sync_dir_socket, deliver_file_port, ST_COMMON,  node.c_str(), verify_dir, getpid());
        bool                sync_success_flag = true;
        if ( sender.sync_start() == false) {
            MSG_DBG_F("dir sync meet error");
            printf("[lurk client] 😰sync dir '%s' meet error\n", node.c_str());
            sync_success_flag = false;
        }
        shutdown(sync_dir_socket, SHUT_RDWR);
        close(sync_dir_socket);

        if (sync_success_flag == false) {
            printf("[lurk client] 🥲sync dir '%s' meet some failures\n", node.c_str());
            // *thread_rtn = EXIT_FAILURE;
            // return thread_rtn;
            continue;
        } else {
            printf("[lurk client] 😀sync dir '%s' all success\n", node.c_str());
            // *thread_rtn = EXIT_SUCCESS;
            // return thread_rtn;
            continue;
        }

    }

}
