#include "deliver/deliver.h"
#include "sys/msg_print.h"
#include "deliver/file_deliver.h"
#include "tools/get_config.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// argv[1] : socktag
// argv[2] : deliver_file_port
// argv[3] : filepath
// argv[4] : basepath_len
// argv[5] : basepath_hash
// argv[6] : verify_dir
int main(int argc, char * argv[])
{
    int                     sockfd = 0;
    struct sockaddr_in      servaddr;

    // argv[1] - argv[6]

    sock_tag_t              socktag;
    char                    server_ip_str[20];
    uint16_t                deliver_file_port;
    char                    filepath[4096];
    uint16_t                basepath_len;
    char                    basepath_hash[65];
    char                    verify_path[4096];



    if (argc != 7) {
        MSG_DBG_F("parameter is not enough, current is %d", argc);
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

    memset(server_ip_str, 0, sizeof(server_ip_str));
    if ( get_config_server_ip_client(server_ip_str) == false) {
        perror("[send single file process]");
        // printf("Could not find 'server_ip' in config file\n");
        // goto failed;
    }
    if ( (deliver_file_port = strtol(argv[2], NULL, 10)) == 0) {
        perror("[send single file process]");
        // 
        // printf("Deliver file port format error\n");
        // goto failed;
    }

    memset(filepath, 0, sizeof(filepath));
    strncpy(filepath, argv[3], strlen(argv[3]));

    basepath_len = strtol(argv[4], NULL, 10);
    if (basepath_len == 0) {
        perror("[send single file process]");
        return EXIT_FAILURE;
    }

    memset(basepath_hash, 0, sizeof(basepath_hash));
    strncpy(basepath_hash, argv[5], strlen(argv[5]));
    basepath_hash[64] = 0;


    memset(verify_path, 0, sizeof(verify_path));
    strncpy(verify_path, argv[6], strlen(argv[6]));

    // server_port = strtol(argv[3], NULL, 10);

    if (socktag == ST_COMMON) {
        if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("[send single file process]");
            goto failed;
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(deliver_file_port);

        if (inet_pton(AF_INET, server_ip_str, &servaddr.sin_addr) != 1) {
            perror("[send single file process]");
            return EXIT_FAILURE;
        }

        if ( (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) == -1) {
            perror("[send single file process]");
            // printf("[-send_single_file_process-]: send file '%s' failed, error is %s\n", filepath, strerror(errno));
            return EXIT_FAILURE;
        }

        // printf("[send single file process] connect server %s::%hu success\n", server_ip_str, deliver_file_port);
    } else if (socktag == ST_UNIX) {

    }
    // char basepath_hash[65];
    // memset(basepath_hash, 0, sizeof(basepath_hash));
    // sha256(basepath_hash, "./send", 6);
    if ( send_single_file(sockfd, socktag, filepath, basepath_len, basepath_hash, verify_path) == false){
        printf("[send single file process]: send file '%s' failed\n", filepath);
        goto failed;
    }

    
    if (sockfd != 0) {
        close(sockfd);
    }
    
    // printf("[send single file process]: send file '%s' success\n", filepath);
    return EXIT_SUCCESS;
failed:
    if (sockfd != 0) {
        close(sockfd);
    }
    return EXIT_FAILURE;
}
