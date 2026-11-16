// #include "core/dir_sync.hpp"
#include "common/msg_print.h"
#include "core/file_deliver.hpp"
#include "common/check_error.hpp"
#include "config/get_config.hpp"
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
#include <time.h>
#include <iostream>

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
try{
    if (argc != 7) {
        throw std::runtime_error("parameter is not enough, current is " + std::to_string(argc));
    }

    if ( strcmp(argv[1], "ST_COMMON") == 0) {
        socktag = ST_COMMON;
    } else if ( strcmp(argv[1], "ST_UNIX") == 0) {
        socktag = ST_UNIX;
    } else {
        throw std::runtime_error("socktag is not specified");
    }

    memset(server_ip_str, 0, sizeof(server_ip_str));
    
    get_config_server_ip(server_ip_str, sizeof(server_ip_str));
    if ( (deliver_file_port = strtol(argv[2], NULL, 10)) == 0) {
        throw std::runtime_error("Deliver file port format error");
    }

    memset(filepath, 0, sizeof(filepath));
    strncpy(filepath, argv[3], strlen(argv[3]));

    basepath_len = strtol(argv[4], NULL, 10);
    if (basepath_len == 0) {
        throw std::runtime_error("basepath_len format error");
    }
    memset(basepath_hash, 0, sizeof(basepath_hash));
    strncpy(basepath_hash, argv[5], strlen(argv[5]));
    basepath_hash[64] = 0;
    
    memset(verify_path, 0, sizeof(verify_path));
    strncpy(verify_path, argv[6], strlen(argv[6]));

    if (socktag == ST_COMMON) {
        sockfd = check_syscall_error(socket(AF_INET, SOCK_STREAM, 0), "socket");
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(deliver_file_port);
        inet_pton(AF_INET, server_ip_str, &servaddr.sin_addr);
        check_syscall_error(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)), "connect");
    } else if (socktag == ST_UNIX) {
        

    }

    if ( send_single_file(sockfd, socktag, filepath, basepath_len, basepath_hash, verify_path) == false){
        printf("[send single file process]: send file '%s' failed\n", filepath);
    }

    close(sockfd);
    
    return EXIT_SUCCESS;
} catch (std::runtime_error & e) {
    std::cerr << "[lurk-client] sync dir catch runtime error => " << e.what() << std::endl;
} catch (std::exception & e) {
    std::cerr << "[lurk-client] sync dir catch other error => " << e.what() << std::endl;
}
    if (sockfd != 0) {
        close(sockfd);
    }
    return EXIT_FAILURE;
}
