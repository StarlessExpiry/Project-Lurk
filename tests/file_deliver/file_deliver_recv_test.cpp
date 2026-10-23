#include "core/dir_sync.hpp"
#include "deliver/deliver.h"
#include "deliver/file_deliver.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>

extern "C" {
    #include "tools/msg_print.h"
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
}
int main (int argc, char *argv[])
{
    struct sockaddr_in  myaddr;
    int                 listenfd;
    
    if ( (listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        MSG_DBG_F("Socket create error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    myaddr.sin_family = PF_INET;


    myaddr.sin_port = htons(20211);
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
    recv_single_file(connfd, ST_COMMON, "./recv", "./recv/verify");

    close(connfd);

}