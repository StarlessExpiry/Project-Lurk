#include "sockaddr_dealer.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
//
// int
// sock_fdtop(int connfd, const char * addr_str)
// {
//         // 获取地址内容后,输出
//         int n;
//         struct sockaddr_in cliaddr_use;
//         socklen_t cliaddr_lenp;
//         if ( (n = getsockname(connfd, (struct sockaddr *)&cliaddr_use, &cliaddr_lenp) == -1)) {
//             msg_err_f("cannot get the address, connfd = %d, and the errno is \"%s\"", connfd, strerror(errno));
//         }
// 		addr_str = inet_ntop(cliaddr_use.sin_family, &cliaddr_use.sin_addr, NULL, sizeof(struct sockaddr)); // 输出
// }

char *
sock_ntop(const struct sockaddr *sa, socklen_t sa_len, char *str, size_t len)
{
    switch (sa->sa_family) {
        case AF_INET: {
            struct sockaddr_in * sin = (struct sockaddr_in *)sa;
            if (inet_ntop(AF_INET, &sin->sin_addr, str, len) ==  NULL)
                return NULL;
            char str_port[8];
            if (ntohs(sin->sin_port) != 0) {
                snprintf(str_port, sizeof(str_port), ":%d", ntohs(sin->sin_port));
                strcat(str, str_port);
            }
            return str;
        }
        case AF_INET6: {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

            // IPv6 格式通常用 [] 包裹端口，如 [2001:db8::1]:80
            str[0] = '[';
            if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, len - 1) == NULL)
                return NULL;
            char str_port[8];
            if (ntohs(sin6->sin6_port) != 0) {
                snprintf(str_port, sizeof(str_port), "]:%d", ntohs(sin6->sin6_port));
                strcat(str, str_port);
            } else {
                strcat(str, "]");
            }
            return str;
        }
        default:
            snprintf(str, len, "unknown family %d", sa->sa_family);
            return str;
    
    }
}
char *
sock_ntop_host_only(const struct sockaddr *sa, socklen_t sa_len, char *str, size_t len)
{

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in * sin = (struct sockaddr_in *)sa;
        if (inet_ntop(AF_INET, &sin->sin_addr, str, len) ==  NULL)
            return NULL;
        return str;
    }
    case AF_INET6: {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

        // IPv6 格式通常用 [] 包裹端口，如 [2001:db8::1]:80
        str[0] = '[';
        // 注意偏移一个字节
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, len - 1) == NULL)
            return NULL;
        strcat(str, "]");
        return str;
    }
    default:
        snprintf(str, len, "unknown family %d", sa->sa_family);
        return str;
    }
}

struct addrinfo **
getaddrinfo_no_hints(const char * hostname, const char * service, int family, int socktype)
{
    struct addrinfo hints = {}, **res = 0; // res只取addrinfo链表的第一个
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = family;
    hints.ai_socktype = socktype;

    if (getaddrinfo(hostname, service, &hints, res) != 0){
        return NULL;
    }
    return res;
}
