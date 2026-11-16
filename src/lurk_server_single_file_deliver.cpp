// this process will be act as a part in lurk_server;
// lurk_server will create about 4 processes to receive single files,
// and lurk_client will tell lurk_server whether single_file sync success or not;
#include "common/check_error.hpp"
#include "core/deliver.h"
#include "core/file_deliver.hpp"
#include "config/get_config.hpp"
#include "common/msg_print.h"
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// argv[1] : socktag, e.g. ST_COMMON...
// argv[2] : deliver_file_port
// argv[3] : recv path
// argv[4] : verify path
int main (int argc, char *argv[])
{
try{
    struct sockaddr_in  myaddr;
    int                 listenfd = 0;
    sock_tag_t          socktag;
    uint16_t            deliver_file_port;
    char                recv_path[4096];
    char                verify_path[4096];

    if (argc != 5)
        throw std::runtime_error("parameter cnt is not match, current is: " + std::to_string(argc));

    if ( strcmp(argv[1], "ST_COMMON") == 0) {
        socktag = ST_COMMON;
    } else if ( strcmp(argv[1], "ST_UNIX") == 0) {
        socktag = ST_UNIX;
    } else {
        throw std::runtime_error("socktag is not specified, current is: " + std::string(argv[1]));
    }


    if ( (deliver_file_port = strtol(argv[2], NULL, 10)) == 0)
        throw std::runtime_error("deliver_file_port format error: " + std::string(strerror(errno)));

    memset(recv_path, 0, sizeof(recv_path));
    strncpy(recv_path, argv[3], strlen(argv[3]));

    memset(verify_path, 0, sizeof(verify_path));
    strncpy(verify_path, argv[4], strlen(argv[4]));

    // file_sync_port = strtol(argv[3], NULL, 10);
    // if (file_sync_port == 0) {
    //     MSG_DBG_F("strtol is failed: %s", strerror(errno));
    //     goto failed;
    // }

    if (socktag == ST_COMMON) {
        listenfd = check_syscall_error(socket(PF_INET, SOCK_STREAM, 0), "socket");
        char        server_ip_str[20];
        memset(server_ip_str, 0, sizeof(server_ip_str));
        get_config_server_ip(server_ip_str, sizeof(server_ip_str));

        myaddr.sin_family = PF_INET;
        myaddr.sin_port = htons(deliver_file_port);
        inet_pton(AF_INET, server_ip_str , &myaddr.sin_addr);

        check_syscall_error(bind(listenfd, (struct sockaddr *)&myaddr, sizeof(myaddr)), "bind");

        check_syscall_error(listen(listenfd, 20), "listen");

        struct sockaddr_in  peeraddr;
        socklen_t           peeraddr_len = sizeof(peeraddr);
        int                 connfd;

        // always waiting for client
        while(1) {
            connfd = check_syscall_error(accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len), "accept");

            struct linger sl;
            sl.l_onoff = 1;
            sl.l_linger = 10; // wait max 10s
            check_syscall_error(setsockopt(connfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)), "setsockopt SO_LINGER");
            // if ( setsockopt(connfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) == false) {
            //     MSG_DBG_F("setsockopt SO_LINGER failed: %s", strerror(errno));
            // } else {
            //     MSG_DBG_F("setsockopt SO_LINGER success");
            // }

            MSG_DBG_F("================= BEGIN recv_single_file ===================");

            if ( recv_single_file(connfd, socktag, recv_path, verify_path) == false) {
                char dummy_buf[128];
                ssize_t n;
                // MSG_DBG("Waiting for client to close connection...");

                while ((n = read(connfd, dummy_buf, sizeof(dummy_buf))) > 0) {
                    // do nothing, just consume the 'rubbish' in socket receive buffer
                }

                // After client close
                if (n == 0) {
                    MSG_DBG("[lurk-server-single-file-deliver] Client closed connection (FIN received). Now closing server side...");
                } else if (n == -1) {
                    MSG_DBG_F("[lurk-server-single-file-deliver] Read error or connection reset: %s", strerror(errno));
                }

                close(connfd);
                throw std::runtime_error("recv_single_file failed");
            }

            MSG_DBG_F("================= END recv_single_file ===================");

            char dummy_buf[128];
            ssize_t n;
            while ((n = read(connfd, dummy_buf, sizeof(dummy_buf))) > 0) {
                // do nothing, just consume the 'rubbish' in socket receive buffer
            }

            // After client close
            if (n == 0) {
                MSG_DBG("[lurk-server-single-file-deliver] Client closed connection (FIN received). Now closing server side...");
            } else if (n == -1) {
                MSG_DBG_F("[lurk-server-single-file-deliver] Read error or connection reset: %s", strerror(errno));
            }
            close(connfd);
            std::cout << "[lurk-server-single-file-deliver] serv a client success !" << std::endl;
        }
    }

    // this will not never be execute;
    if (listenfd > 0)
        close(listenfd);
    MSG_DBG("Server process exit");
    return EXIT_SUCCESS;

// failed:
//     if (listenfd > 0)
//         close(listenfd);
//     MSG_DBG("Server process failed, exit");
//     return EXIT_FAILURE;
}catch (std::runtime_error& e) {
    std::cerr << "[lurk-server-single-file-deliver] failed : " << e.what() << std::endl;
} catch (std::exception& e) {
    std::cerr << "[lurk-server-single-file-deliver] failed : " << e.what() << std::endl;
}
    return EXIT_FAILURE;
}
