#include "common/check_error.hpp"
#include "core/dir_sync.hpp"
#include "config/get_config.hpp"
#include "common/msg_print.h"
#include <iostream>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


// argv[1] : port
// argv[2] : leader_pid
int main (int argc, char *argv[])
{
    struct sockaddr_in  myaddr;
    int                 listenfd, connfd;
    char                server_ip_str[20];
    pid_t               leader_pid;
    struct sockaddr_in  peeraddr;
    socklen_t           peeraddr_len = sizeof(peeraddr);
    fd_set              readfds;
    struct timeval      tv;
    Sync_dir_receiver * receiver = nullptr;
try {
    if ( argc != 3) {
        throw std::runtime_error("parameter cnt error, current is " + std::to_string(argc));
    }

    myaddr.sin_family = PF_INET;

    short port = strtol(argv[1], NULL, 10);
    if ( port == 0) {
        throw std::runtime_error("port error: " + std::string(strerror(errno)));
    }
    leader_pid = strtol(argv[2], NULL, 10);
    if (leader_pid == 0) {
        leader_pid = getpid();
    }

    myaddr.sin_port = htons(port);
    memset(server_ip_str, 0, sizeof(server_ip_str));
    get_config_server_ip(server_ip_str, sizeof(server_ip_str));
    inet_pton(AF_INET, server_ip_str, &myaddr.sin_addr);


    listenfd = check_syscall_error(socket(PF_INET, SOCK_STREAM, 0), "socket");
    check_syscall_error(bind(listenfd, (struct sockaddr *)&myaddr, sizeof(myaddr)), "bind");
    check_syscall_error(listen(listenfd, 20), "listen");

    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    tv.tv_sec = 30;  // wait 30s max
    tv.tv_usec = 0;

    // use select to wait for a fixed time
    MSG_DBG_F("[lurk-server-dir-sync] accept waiting...");
    int ret = select(listenfd+1, &readfds, NULL, NULL, &tv);
    if (ret < 0) {
        throw std::runtime_error("select() error: " + std::string(strerror(errno)));
    } else if (ret == 0) {
        throw std::runtime_error("select() timeout: no incoming connection within 30 seconds");
    } else {
        // recv a connection!
        if (FD_ISSET(listenfd, &readfds)) {
            connfd = check_syscall_error(accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len), "accept");
        }
    }

    printf("[lurk-server-dir-sync] server_ip = %s, server_port = %hu, start dir sync\n", server_ip_str, port);

    receiver = Sync_dir_receiver_builder()
                    .set_sockfd(connfd)
                    .set_socktag(ST_COMMON)
                    .set_leader_pid(leader_pid)
                    .build();
    bool recv_success_flag = true;
    if ( receiver->sync_start() == false) {
        recv_success_flag = false;
    }

    char tmp[10];
    while ( read(connfd, tmp, 10) > 0) {
        // do nothing
    } // until read() == 0 or -1
    close(connfd);

    if (recv_success_flag == true) {
        std::cout << "[lurk-server-dir-sync] a dir sync success 😀!" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "[lurk-server-dir-sync] a dir sync failed😞" << std::endl;
        return EXIT_FAILURE;
    }
    if (receiver != nullptr) delete receiver;
} catch (std::runtime_error& e) {
    std::cerr << "[lurk-server-dir-sync] failed : " << e.what() << std::endl;
} catch (std::exception& e) {
    std::cerr << "[lurk-server-dir-sync] failed : " << e.what() << std::endl;
}
    if (receiver != nullptr) delete receiver;
}
