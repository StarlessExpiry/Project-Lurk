#include "deliver/deliver.h"
#include "deliver/file_deliver.h"
#include "tools/get_config.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string.h>

extern "C" {
    #include "sys/msg_print.h"
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
}

// argv[1] : socktag, e.g. ST_COMMON...
// argv[2] : deliver_file_port
// argv[3] : recv path
// argv[4] : verify path
int main (int argc, char *argv[])
{
    struct sockaddr_in  myaddr;
    int                 listenfd = 0;
    sock_tag_t          socktag;
    uint16_t            deliver_file_port;
    char                recv_path[4096];
    char                verify_path[4096];

    if (argc != 5) {
        MSG_DBG_F("parameter cnt is not match, current is: %d", argc);
        return EXIT_FAILURE;
    }

    if ( strcmp(argv[1], "ST_COMMON") == 0) {
        socktag = ST_COMMON;
    } else if ( strcmp(argv[1], "ST_UNIX") == 0) {
        socktag = ST_UNIX;
    } else {
        MSG_DBG_F("socktag is not specified");
        return EXIT_FAILURE;
    }

    if ( (deliver_file_port = strtol(argv[2], NULL, 10)) == 0) {
        // printf("Could not find 'server_ip' in config file\n");
        MSG_DBG_F("deliver_file_port format error");
        goto failed;
    }

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

        if ( (listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
            MSG_DBG_F("Socket create error: %s", strerror(errno));
            return EXIT_FAILURE;
        }

        char        server_ip_str[20];

        memset(server_ip_str, 0, sizeof(server_ip_str));
        if ( get_config_server_ip_server(server_ip_str) == false) {
            printf("Could not find 'server_ip' in config file\n");
            goto failed;
        }


        myaddr.sin_family = PF_INET;
        myaddr.sin_port = htons(deliver_file_port);
        inet_pton(AF_INET, server_ip_str , &myaddr.sin_addr);


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


        // always waiting for client
        while(1) {
            if ( (connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len)) == -1) {
                MSG_DBG_F("Accept failed: %s", strerror(errno));
                return EXIT_FAILURE;
            }

            struct linger sl;
            sl.l_onoff = 1;
            sl.l_linger = 10; // wait max 10s
            if ( setsockopt(connfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl)) == false) {
                MSG_DBG_F("setsockopt SO_LINGER failed: %s", strerror(errno));
            } else {
                MSG_DBG_F("setsockopt SO_LINGER success");
            }

            MSG_DBG_F("================= BEGIN recv_single_file ===================");

            if ( recv_single_file(connfd, socktag, recv_path, verify_path) == false) {
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
                goto failed;
            }


            MSG_DBG("Waiting for client to close connection...");
            char dummy_buf[128];
            ssize_t n;
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
            MSG_DBG_F("Waiting for next connection");
        }
    }

    if (listenfd > 0)
        close(listenfd);
    MSG_DBG("Server process exit");
    return EXIT_SUCCESS;

failed:
    if (listenfd > 0)
        close(listenfd);
    MSG_DBG("Server process failed, exit");
    return EXIT_FAILURE;
}
