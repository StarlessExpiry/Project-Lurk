#include "core/dir_sync.hpp"
#include "deliver/deliver.h"
#include "deliver/file_deliver.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <spawn.h>
#include <string.h>
#include <unistd.h>

extern "C" {
    #include "sys/msg_print.h"
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
}


extern char ** environ;

int main (int argc, char *argv[])
{

    // Pe-created process to recv file
    char *  argv_str[6];

    argv_str[0] = (char *)"lurk_server_single_file_deliver";
    argv_str[1] = (char *)"ST_COMMON";
    argv_str[2] = (char *)"20211";
    argv_str[3] = (char *)"./recv";
    argv_str[4] = (char *)"./recv/verify";
    argv_str[5] = NULL;

    pid_t pid_rtn;
    posix_spawnattr_t process_attr;

    posix_spawnattr_init(&process_attr);
    posix_spawnattr_setflags(&process_attr, POSIX_SPAWN_SETPGROUP);
    posix_spawnattr_setpgroup(&process_attr, getpid());
    if ( posix_spawn(&pid_rtn, "./lurk_server_single_file_deliver", NULL, &process_attr, argv_str, environ) != 0) {
        MSG_DBG_F("create process errro : %s", strerror(errno));
        return EXIT_FAILURE;
    }
    MSG_DBG_F("create process success, pid = %d", pid_rtn);

    struct sockaddr_in  myaddr;
    int                 listenfd;

    if ( (listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        MSG_DBG_F("Socket create error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    myaddr.sin_family = PF_INET;


    myaddr.sin_port = htons(20200);
    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr);


    if ( bind(listenfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
        MSG_DBG_F("Bind error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    if ( listen(listenfd, 20) == -1) {
        MSG_DBG_F("Listen error: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    struct sockaddr_in  peeraddr;
    socklen_t           peeraddr_len = sizeof(peeraddr);
    int                 connfd;
    if ( (connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len)) == -1) {
        MSG_DBG_F("Accept failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    MSG_DBG("One client");

    struct linger sl;
    sl.l_onoff = 1;
    sl.l_linger = 10; // wait 10s
    if ( setsockopt(connfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) == false) {
        MSG_DBG_F("setsockopt SO_LINGER failed: %s", strerror(errno));
    } else {
        MSG_DBG_F("setsockopt SO_LINGER success");
    }

    Sync_dir_receiver receiver(connfd,  ST_COMMON, getpid());
    receiver.sync_start();

    char dummy_buf[128];
    ssize_t n;
    MSG_DBG("Waiting for client to close connection...");

    while ((n = read(connfd, dummy_buf, sizeof(dummy_buf))) > 0) {
        // do nothing, just consume the 'rubbish' in socket receive buffer
    }

    // After client close
    if (n == 0) {
        MSG_DBG("Client closed connection (FIN received). Now closing server side...");
    } else if (n == -1) {
        MSG_DBG_F("Read error or connection reset: %s", strerror(errno));
    }

    close(connfd);
    close(listenfd);

    MSG_DBG("Server process exiting cleanly.");
    return EXIT_SUCCESS;
}
