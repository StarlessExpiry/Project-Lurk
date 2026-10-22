#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>


/*
 * 作用:    将网络地址转换为字符串
 * 参数:    struct sockaddr_in * sa 待传入待地址结构
 *          socklen_t sa_len        地址结构长度
 *          char * str              地址将写入的缓冲区（必须传入，不然段错误）
 *          size_t len              缓冲区长度
 * 返回值:  int 正确时返回0，错误时返回-1（UNIX系统调用风格）
 */
char *
sock_ntop(const struct sockaddr *sa, socklen_t sa_len, char *str, size_t len);

char *
sock_ntop_host_only(const struct sockaddr *sa, socklen_t sa_len, char *str, size_t len);

/*
 * 作用:    自动构建hints的getaddrinfo
 * 参数:    const char * hostname   主机名
 *          const char * service    服务名
 *          int family              协议族
 *          int socktype            SOCK_STREAM, SOCK_DGRAM, 0, etc
 * 返回值:  同getaddrinfo，返回指向addrinfo结构体链表的指针，通过addrinfo的ai_next成员进行遍历,
 *          返回值仍然需要调用freeaddrinfo(struct addrinfo * ai)传入链表首元素进行释放
 *
 */
struct addrinfo **
getaddrinfo_no_hints(const char * hostname, const char * service, int family, int socktype);
