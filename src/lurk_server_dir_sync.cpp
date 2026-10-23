#include "core/dir_sync.hpp"
#include "tools/get_config.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string.h>

extern "C" {
    #include "tools/msg_print.h"
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
}

// argv[1] : port
// argv[2] : leader_pid
int main (int argc, char *argv[])
{
    struct sockaddr_in  myaddr;
    int                 listenfd;
    char                server_ip_str[20];

    if ( argc != 3) {
        MSG_DBG("parameter cnt error");
        return EXIT_FAILURE;
    }

    myaddr.sin_family = PF_INET;

    short port = strtol(argv[1], NULL, 10);
    if ( port == 0) {
        MSG_DBG_F("Port error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    pid_t leader_pid = strtol(argv[2], NULL, 10);
    if (leader_pid == 0) {
        // MSG_DBG_F("Leader pid error: %s", strerror(errno));
        // return EXIT_FAILURE;
        leader_pid = getpid();
    }


    myaddr.sin_port = htons(port);

    memset(server_ip_str, 0, sizeof(server_ip_str));
    if ( get_config_server_ip_server(server_ip_str) == false) {
        printf("Get server_ip in your config file failed.\n");
        return EXIT_FAILURE;
    }
    inet_pton(AF_INET, server_ip_str, &myaddr.sin_addr);

    printf("server_ip = %s, server_port = %hu\n", server_ip_str, port);

    if ( (listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        MSG_DBG_F("Socket create error: %s", strerror(errno));
        return EXIT_FAILURE;
    }

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
    MSG_DBG("Begin accept");
    

    fd_set              readfds;
    struct timeval      tv;

    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    tv.tv_sec = 30;  // wait 30s max
    tv.tv_usec = 0;

    int ret = select(listenfd+1, &readfds, NULL, NULL, &tv);
    if (ret < 0) {
        MSG_DBG_F("select error: %s", strerror(errno));
        return EXIT_FAILURE;
    } else if (ret == 0) {
        MSG_DBG_F("Time out");
        return EXIT_FAILURE;
    } else {
        if (FD_ISSET(listenfd, &readfds)) {
            if ( (connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len)) == -1) {
                MSG_DBG_F("Accept failed: %s", strerror(errno));
                return EXIT_FAILURE;
            }
        }
    }

    // if ( (connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len)) == -1) {
    //     MSG_DBG_F("Accept failed: %s", strerror(errno));
    //     return EXIT_FAILURE;
    // }
    MSG_DBG("One client");

    // char verify_path[4096];
    // memset(verify_path, 0, sizeof(verify_path));
    // get_config_verifypath_server(verify_path);

    Sync_dir_receiver       receiver(connfd, ST_COMMON, leader_pid);
    bool                    recv_success_flag = true;
    if ( receiver.sync_start() == false) {
        recv_success_flag = false;
    }

    char tmp[10];
    while ( read(connfd, tmp, 10) > 0) {
        // do nothing
    } // until read() == 0 or -1

    close(connfd);

    if (recv_success_flag == true) {
        MSG_DBG_F("Recveive dir success");
        return EXIT_SUCCESS;
    } else {
        MSG_DBG_F("Recveive dir failed");
        return EXIT_FAILURE;
    }
}
