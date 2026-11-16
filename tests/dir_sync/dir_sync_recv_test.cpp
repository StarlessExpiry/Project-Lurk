#include "common/check_error.hpp"
#include "core/dir_sync.hpp"
#include "core/deliver.h"
#include "common/msg_print.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <spawn.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>




extern char ** environ;

int main (int argc, char *argv[])
{
    Sync_dir_receiver * receiver = nullptr;
    pid_t               pid_rtn;
    posix_spawnattr_t   process_attr;
    struct sockaddr_in  myaddr;
    int                 listenfd;
    struct sockaddr_in  peeraddr;
    socklen_t           peeraddr_len = sizeof(peeraddr);
    int                 connfd;
try {

    // Pe-created process to recv file
    char *  argv_str[6];

    argv_str[0] = (char *)"lurk_server_single_file_deliver";
    argv_str[1] = (char *)"ST_COMMON";
    argv_str[2] = (char *)"20211";
    argv_str[3] = (char *)"./recv";
    argv_str[4] = (char *)"./recv/verify";
    argv_str[5] = NULL;

    posix_spawnattr_init(&process_attr);
    posix_spawnattr_setflags(&process_attr, POSIX_SPAWN_SETPGROUP);
    posix_spawnattr_setpgroup(&process_attr, getpid());

    check_posix_thread_error(posix_spawn(&pid_rtn, "./lurk_server_single_file_deliver", NULL, &process_attr, argv_str, environ), "posix_spawn");
    MSG_DBG_F("create process success, pid = %d", pid_rtn);

    listenfd = check_syscall_error(socket(PF_INET, SOCK_STREAM, 0), "socket");

    // if ( (listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    //     MSG_DBG_F("Socket create error: %s", strerror(errno));
    //     return EXIT_FAILURE;
    // }
    myaddr.sin_family = PF_INET;
    myaddr.sin_port = htons(20200);
    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr);

    check_syscall_error(bind(listenfd, (struct sockaddr *)&myaddr, sizeof(myaddr)), "bind");

    check_syscall_error(listen(listenfd, 20), "listen");

    MSG_DBG("[lurk-server-dir-sync] accept waiting...");
    connfd = check_syscall_error(accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len), "accept");
    MSG_DBG("[lurk-server-dir-sync] One client");

    struct linger sl;
    sl.l_onoff = 1;
    sl.l_linger = 10; // wait 10s
    if ( setsockopt(connfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) == false) {
        MSG_DBG_F("[lurk-server-dir-sync] setsockopt SO_LINGER failed: %s", strerror(errno));
    } else {
        MSG_DBG_F("[lurk-server-dir-sync] setsockopt SO_LINGER success");
    }

    // Sync_dir_receiver receiver(connfd,  ST_COMMON, getpid());
    receiver = Sync_dir_receiver_builder()
                    .set_sockfd(connfd)
                    .set_socktag(ST_COMMON)
                    .set_leader_pid(getpid())
                    .build();
    receiver->sync_start();

    MSG_DBG("[lurk-server-dir-sync] Waiting for client to close connection...");

    char dummy_buf[128];
    ssize_t read_rtn;
    while ((read_rtn = read(connfd, dummy_buf, sizeof(dummy_buf))) > 0) {
        // do nothing, just consume the 'rubbish' in socket receive buffer
    }

    // After client close
    if (read_rtn == 0) {
        MSG_DBG_F("[lurk-server-dir-sync] Client closed connection (FIN received). Now closing server side...");
    } else if (read_rtn == -1) {
        MSG_DBG_F("[lurk-server-dir-sync] Read error or connection reset: %s", strerror(errno));
    }

    close(connfd);
    close(listenfd);

    MSG_DBG("[lurk-server-dir-sync] Server process exiting cleanly.");
    return EXIT_SUCCESS;
} catch (std::runtime_error& e) {
    std::cerr << "[lurk-server-dir-sync] failed : " << e.what() << std::endl;
} catch (std::exception& e) {
    std::cerr << "[lurk-server-dir-sync] failed : " << e.what() << std::endl;
}
    return EXIT_FAILURE;
}
