#include "sys/msg_print.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <unistd.h>

bool get_available_ip(char ip_str[NI_MAXHOST]) {
    struct ifaddrs * ifaddr;

    if ( getifaddrs(&ifaddr) == -1) {
        MSG_DBG_F("getifaddrs failed: %s", strerror(errno));
        return false;
    }

    bool get_success_flag = false;
    // traverse link list
    struct ifaddrs * ifa;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if ( ifa->ifa_addr == NULL || !(ifa->ifa_flags & IFF_UP)) {
            continue;
        }
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) { // only ipv4 now
            int error = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip_str, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (error != 0) {
                MSG_DBG_F("getnameinfo error: %s", strerror(error));
                continue;
            }
            // get success
            get_success_flag = true;
            break;
        }
    }

    if (get_success_flag == true)
        return true;
    else
        return false;
}

uint16_t get_available_port() {
    int                 sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in  myaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(0)
    };
    int                 port;

    if ( bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
        MSG_DBG_F("Bind error: %s", strerror(errno));
        return 0;
    }

    socklen_t           addr_len = sizeof(myaddr);
    getsockname(sockfd, (struct sockaddr *)&myaddr, &addr_len);

    // get an avaliable port
    port = ntohs(myaddr.sin_port);

    close(sockfd);
    return port;
}
