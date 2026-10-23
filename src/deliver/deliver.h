#pragma once
#include "tools/msg_print.h"
#include "tools/sha.h"
#include "deps/sqlite/sqlite3.h"
#include <fts.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#if defined (__APPLE__) || defined (__FreeBSD__)
#include <arpa/inet.h>          // 提供htonll和ntohll
#elif defined (__linux__)
#include <endian.h>             // 提供htobe64（主机->网络64位）和be64toh
#endif


/**
 * @brief
 */
// gfy_stat_t
// send_single_file(int sockfd, const char * path_to_send);

// gfy_stat_t
// recv_single_file(int sockfd, const char * path_to_recv);

// gfy_stat_t
// send_files_to(int sockfd, const char * dir_to_send, const char * verify_dir, connection_pool_t * const connection_pool);







// #define DEFAULT_HASH_BLOCK_LEN (64 * 100) // 最多有100个

// for macos10.5以上, 使用darwin上64-bit的stat（包含创建时间）
#ifndef _DARWIN_USE_64_BIT_INODE
#define _DARWIN_USE_64_BIT_INODE
#endif

// 默认接受的文件数据块大小为128KB
#ifndef RECV_BUFD_SIZE
#define RECV_BUF_SIZE (128 * 1024)
#endif

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(1)

// Message header for receiver to read
typedef struct {
    int         dh_tag;
    size_t      dh_data_len;
} deliver_header_t;

#pragma pack()

typedef enum {
    ST_COMMON,
    ST_LAN,
    ST_UNIX
} sock_tag_t;

// bool
// deliver_config(int sockfd, sock_tag_t socktag);
//
// bool
// deliver(int header_tag, const void * deliver_data, size_t deliver_data_len);
//
// bool
// deliver_scatter(int hedear_tag, const struct iovec * data_iov, int data_iov_cnt);
//
// // bool
// // deliver_peek_header(int sockfd, sock_tag_t sock_tag, deliver_header_t * header, int * empty_flag);
//
// // bool
// // deliver_get_header(deliver_header_t * header);
// bool
// deliver_get_header(deliver_header_t * header, int * empty_flag);
//
//
// bool
// deliver_get_data(void * data, int data_len, int * empty_flag);

bool
deliver(int sockfd, sock_tag_t socktag, int header_tag, const void * deliver_data, size_t deliver_data_len);

bool deliver_scatter(int sockfd, sock_tag_t socktag, int header_tag, const struct iovec * data_iov, int data_iov_cnt);

bool
deliver_peek_header(int sockfd, sock_tag_t socktag, deliver_header_t * header,int * empty_flag);

bool
deliver_get_header(int sockfd, sock_tag_t socktag, deliver_header_t * header, int * empty_flag);

bool
deliver_get_data(int sockfd, sock_tag_t socktag, void * data, int data_len, int * empty_flag);




// // The data which client should send to server, and server need to parse it to compare local database;
// // After server compare all the entrys,
// // he should send data which type is database_entrys_need_t to request send_single_file process about specific file from client.
// typedef struct database_entry_set {
//     int     de_entry_size;
//     char    de_hash_names[100][65];
//     int     de_modify_times[100];
//     int     de_file_sizes[100];
//     short   de_file_flags[100];
// } database_entry_set_t;

// // for server(receive process), which needs to send the hash names to request client to send files
// typedef struct database_entrys_need {
//     int     den_entry_size;
//     char    den_hash_names[65][100];
// } database_entrys_need_t;
#ifdef __cplusplus
}
#endif
