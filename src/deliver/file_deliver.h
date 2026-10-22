#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "deliver.h"
#include "../sys/msg_print.h"
#include "../sys/io.h"
#include "../sys/syserrno.h"
#include "../tools/sha.h"
#include "../deps/sqlite/sqlite3.h"
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
#include <arpa/inet.h>







// #define DEFAULT_BLOCK_LEN (64 * 1,000)


// bool
// send_single_file(int sockfd, sock_tag_t socktag, const char * path_to_send, const char * verify_dir);
bool
send_single_file(int sockfd, sock_tag_t socktag, const char * filepath, int basepath_len, char basepath_hash[65], const char * verify_dir);

bool
recv_single_file(int sockfd, sock_tag_t socktag, const char * path_to_recv, const char * verify_dir);


// fh_type取值
typedef enum {
    FI_REGULAR,
    FI_CANCEL
} fi_type_t;

enum deliver_file_type: uint8_t {
    DHT_FILE_INFORMATION,
    DHT_FILE_BLOCK,
    DHT_FILE_VERIFY_BLOCK,
    DHT_FILE_VB_ACK,
    DHT_FILE_STOP_SEND
};

#pragma pack(1)

typedef struct  {
    fi_type_t           fi_type;
    uint64_t            fi_file_size;
    uint64_t            fi_verify_size;
    uint32_t            fi_mode;
    uint64_t            fi_mtime_sec;
    uint32_t            fi_block_len_set;
    char                fi_filepath[4096];
    uint16_t            fi_basepath_len;
    char                fi_basepath_hash[65];
} file_info_t;


typedef struct  {
    // char        fdb_pathname_hash[64];  // 文件名的hash值，根据pathname来创建唯一的hash值
    char        fdb_data_hash[64];
    int         fdb_no;                 // 块的编号, 从0开始
    // int         fdb_remain;             // 剩余的数据块数, 每一次发送都会更新
    int         fdb_data_len;
    // int32_t     fdb_data_len_set;       // 发送端设置发送时的块大小
    char        fdb_data[];            // 一次读取-发送最多256KB
} file_data_block_t;

typedef enum {
    FI_NO_NEED_DELIVER,
    FI_FAILED,
    FI_NEED_COMPARE,
    FI_NEED_ALL
} fi_stat_t;

// 用于回复
typedef struct {
    fi_stat_t   fi_stat;
    // char        fi_msg[20];     // 如果失败, 则此成员返回字符串格式失败信息
} file_info_ack_t;


/**
 * @brief   存储本次传输任务需要传输的块号
 */
typedef struct {
    char        vb_pathname_hash[64];
    int         vb_list_len;
    int         vb_list[100]; // 每一次服务端最多发送回能传输的100个
} verify_block_ack_t;

#pragma pack()

#ifdef __cplusplus
}
#endif
