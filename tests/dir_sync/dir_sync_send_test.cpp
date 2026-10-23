#include "core/dir_sync.hpp"
#include "deliver/deliver.h"
#include "tools/msg_print.h"
#include "deliver/file_deliver.h"
#include "tools/sha.h"
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int                     sockfd;
    struct sockaddr_in      servaddr;
    char                    verify_path[4096];

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        MSG_DBG_F("Socket create failed: %s", strerror(errno));
        return EXIT_FAILURE;
    }


    memset(verify_path, 0, sizeof(verify_path));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(20200);
    if (inet_pton(AF_INET, "127.0.0.1",
                  &servaddr.sin_addr) != 1) {
        perror("inet_pton");
        return EXIT_FAILURE;
    }


    if ( (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
        MSG_DBG("connect failed");
        return EXIT_FAILURE;
    }
    // char basepath_hash[65];
    // memset(basepath_hash, 0, sizeof(basepath_hash));
    // sha256(basepath_hash, "./send", 6);
    Sync_dir_sender sender(sockfd, 20211, ST_COMMON, "/Users/starless/Projects/Sync-Files-System/tests/send", "./send/verify", getpid());
    sender.sync_start();
    close(sockfd);
}
