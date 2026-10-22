#include "file_deliver.h"
#include "deliver.h"
#include "../flags.h"
#include "../tools/makedir.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>

// static pthread_key_t  pkey1;
// static pthread_once_t pkeyl_once = PTHREAD_ONCE_INIT;




typedef struct block_no_node{
    int number;
    struct block_no_node * next;
} block_no_node_t;

typedef struct {
    block_no_node_t     head;
    size_t              list_len;
    size_t              max_list_len;
    // pthread_mutex_t     mutex;
} block_send_list;


typedef struct {
    int              sockfd;
    sock_tag_t       socktag;
    // for deliver
    size_t           message_len;
    // for deliver
    int              message_empty_flag;
    // use a fixed-size file blocks, so the
    uint32_t         block_len_set;
    // Absolute path to your file
    char             file_path[4096];
    // Absolute path to your verify_file_path which corresponding your file
    char             verify_path[4096];
    // e.g. current sending file is ~/send/your_file, the basepath is ~/send, length of it is 6
    uint16_t         basepath_len;
    // basepath_hash should get from database (by the Caller process)
    char             basepath_hash[65];

    FILE *           file_stream,     * verify_stream;
    struct stat      file_stat,         verify_stat;

    block_send_list  send_list;
} tsd_t;

static pthread_key_t    tsd_key;
static pthread_once_t   tsd_once_key  = PTHREAD_ONCE_INIT;

static void
tsd_destructor(void * ptr)
{
    // tsd_t * tsd = ptr;
    // pthread_mutex_destroy(&tsd->send_list.mutex);

    free(ptr);
}

static void
tsd_once()
{
    pthread_key_create(&tsd_key, tsd_destructor);
}



enum PROCESS_INDICATOR {
    PROCESS_HANDLE_INFO,
    PROCESS_SEND_BLOCK,
    PROCESS_SEND_VERIFY,
    PROCESS_HANDLE_BLOCK_NEED,
};


static bool
send_info();

// static bool
// get_info_ack();
static bool
get_info_ack(file_info_ack_t * ack);

static bool
send_block(int block_no);

static bool
send_verify();

static bool
recv_block_need();

static bool
send_block_need();



// send时，认为单个文件是最新的
// verify_dir need to set as "./verify/12cesdfadsfdsfnkdflkjlkjfa.../"
bool
send_single_file(int sockfd, sock_tag_t socktag, const char * filepath, int basepath_len, char basepath_hash[65], const char * verify_dir)
{
    MSG_DBG("Sending single file");

    int                 maxfd = sockfd;
    fd_set              rset, wset;
    int                 nready;
    // deliver_header_t    header;
    tsd_t *             tsd;

    enum PROCESS_INDICATOR    process_indicator = PROCESS_HANDLE_INFO;


    pthread_once(&tsd_once_key, tsd_once);
    if ( (tsd = pthread_getspecific(tsd_key)) == NULL) {
        tsd = calloc(1, sizeof(tsd_t));
        pthread_setspecific(tsd_key, tsd);
    }
    tsd->sockfd = sockfd;
    tsd->socktag = socktag;

    // INIT ALL VARIABLES
    tsd->message_len = 0;
    tsd->message_empty_flag = 0;
    tsd->block_len_set = DEFAULT_BLOCK_LEN;

    strncpy(tsd->file_path, filepath, sizeof(tsd->file_path));

    memset(tsd->verify_path, 0, sizeof(tsd->verify_path));
    snprintf(tsd->verify_path, sizeof(tsd->verify_path), "%s/%s/%s", verify_dir, basepath_hash, filepath+basepath_len+1);

    // for sending to reciver (by sendinfo())
    tsd->basepath_len = basepath_len;
    memset(tsd->basepath_hash, 0, sizeof(tsd->basepath_hash));
    memcpy(tsd->basepath_hash, basepath_hash, 64);

    tsd->file_stream = NULL, tsd->verify_stream = NULL;

    // if (sockfd != 0)
    //     deliver_config(sockfd, socktag);

    make_dir(tsd->file_path);
    if ( (tsd->file_stream = fopen(tsd->file_path, "rb+")) == NULL) {
        msg_err_f("Open file \"%s\" meet an error: \"%s\"", tsd->file_path, strerror(errno));
    }

    make_dir(tsd->verify_path);
    if ( (tsd->verify_stream = fopen(tsd->verify_path, "r+")) == NULL) {
        MSG_DBG_F("Open verify_file : %s failed: %s", tsd->verify_path, strerror(errno));
        goto failed;
    } else {
        if ( stat(tsd->file_path, &tsd->file_stat) == -1) {
            MSG_DBG_F("stat error: %s", strerror(errno));
            goto failed;
        }

        if ( stat(tsd->verify_path, &tsd->verify_stat) == -1) {
            MSG_DBG_F("stat error: %s", strerror(errno));
            goto failed;
        }

        uint64_t   verify_size_expect;
        if (tsd->file_stat.st_size % tsd->block_len_set == 0) {
            verify_size_expect =  tsd->file_stat.st_size / tsd->block_len_set * 64;
        } else {
            verify_size_expect =  tsd->file_stat.st_size / tsd->block_len_set * 64 + 64;
        }

        if (tsd->verify_stat.st_mtimespec.tv_sec != tsd->file_stat.st_mtimespec.tv_sec ||
            tsd->verify_stat.st_size != verify_size_expect) {
            MSG_DBG_F("verify file is not Lastest, need update it");
            goto failed;
        }
    }

    // pthread_mutex_init(&tsd->send_list.mutex, NULL);



    tsd->send_list.head.next = NULL;
    tsd->send_list.list_len = 0;
    tsd->send_list.max_list_len = 0;

    int    send_flag = 1;
    int    read_flag = 0;
    while (1) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        if (process_indicator == PROCESS_HANDLE_INFO && send_flag == 1) {
            FD_SET(sockfd, &wset);
        }
        if (process_indicator == PROCESS_HANDLE_INFO && read_flag == 1) {
            FD_SET(sockfd, &rset);
        }

        if (process_indicator == PROCESS_SEND_BLOCK ||
            process_indicator == PROCESS_SEND_VERIFY) {
            FD_SET(sockfd, &wset);
        }
        if (process_indicator == PROCESS_HANDLE_BLOCK_NEED && send_flag == 1) {
            FD_SET(sockfd, &wset);
        }
        if (process_indicator == PROCESS_HANDLE_BLOCK_NEED && read_flag == 1) {
            FD_SET(sockfd, &rset);
        }

        MSG_DBG("Select wait");
        if ( (nready = select(maxfd+1, &rset, &wset, NULL, NULL)) == -1) {
            if (errno == EINTR) {
                continue;
            }
        }
        if (FD_ISSET(sockfd, &wset)) {
            switch (process_indicator) {
                case PROCESS_HANDLE_INFO: {
                    MSG_DBG("Turn into PROCESS_INFORMATION_SEND");
                    MSG_DBG_F("Send info about \"%s\"", filepath);

                    if ( send_info() == false)
                        goto failed;
                    else {
                        MSG_DBG("Send info success, waiting ack");
                        send_flag = 0;
                        read_flag = 1;
                    }
                    break;
                }
                case PROCESS_SEND_BLOCK: {
                    MSG_DBG("Turn into PROCESS_SEND_BLOCK");
                    int i;
                    for (i = 0; i < tsd->file_stat.st_size / tsd->block_len_set; i++) {
                        printf("%s: [ %3d%% ]\r", filepath, (i * 100)/(int)(tsd->file_stat.st_size/tsd->block_len_set));
                        fflush(stdout);
                        if ( send_block(i) == false)
                            goto failed;
                    }
                    if ( tsd->file_stat.st_size % tsd->block_len_set != 0) {
                        if ( send_block(i) == false)
                            goto failed;
                    }
                    printf("%s: [ %3d%% ]\n", filepath, 100);
                    deliver(tsd->sockfd, tsd->socktag, DHT_FILE_BLOCK, NULL, 0); // send finish inform
                    // send_block(0);
                    goto success;
                }
                case PROCESS_SEND_VERIFY: {
                    MSG_DBG("Turn into PROCESS_SEND_VERIFY");

                    if ( send_verify() == false)
                        goto failed;
                    process_indicator = PROCESS_HANDLE_BLOCK_NEED;
                    send_flag = 0; // recv first
                    read_flag = 1;
                    break;
                }
                case PROCESS_HANDLE_BLOCK_NEED : {
                    MSG_DBG("Turn into PROCESS_HANDLE_BLOCK_NEED (send)");
                    if ( tsd->send_list.head.next == NULL) {
                        if (read_flag == 0) {
                            if ( deliver(tsd->sockfd, tsd->socktag, DHT_FILE_BLOCK, NULL, 0) == false) {
                                MSG_DBG_F("Deliver failed");
                                goto failed;
                            }
                            goto success;
                        }
                        send_flag = 0; // stop send
                        break;
                    }

                    int percentage = (int)(tsd->send_list.max_list_len-tsd->send_list.list_len)/tsd->send_list.max_list_len * 100;
                    if (percentage != 100) {
                        printf("%s: remain [ %3d%% ]\r", filepath, percentage);
                        fflush(stdout);
                    } else {
                        printf("%s: remain [ %3d%% ]\n", filepath, 100);
                    }

                    if ( send_block_need() == false) {
                        MSG_DBG_F("send_block_need failed");

                        goto failed;
                    }
                    // send_flag = 1;
                    break;
                }
                default: {
                    MSG_DBG("Error state");
                    goto failed;
                }
            }
        }
        if (FD_ISSET(sockfd, &rset)) {
             deliver_header_t header;
             int              empty_flag = 0;
             if ( deliver_get_header(tsd->sockfd, tsd->socktag, &header, &empty_flag) == false) {
                 if (empty_flag == 1) {
                     MSG_DBG("Server exit");
                     goto failed;
                 } else {
                     MSG_DBG_F("Socket error: %s", strerror(errno));
                     goto failed;
                 }
             } else {
                 tsd->message_len = ntohll(header.dh_data_len);
             }
             if ( header.dh_tag == DHT_FILE_STOP_SEND) {
                 MSG_DBG_F("Server failed, quit");
                 goto failed;
             }

             switch (process_indicator) {
                 case PROCESS_HANDLE_INFO: {
                     MSG_DBG("Turn into PROCESS_HANDLE_INFO (recv)");
                     file_info_ack_t    info_ack;
                     if ( get_info_ack(&info_ack) == false) {
                         goto failed;
                     }
                     if (info_ack.fi_stat == FI_NO_NEED_DELIVER) {
                         MSG_DBG("No need deliver, quit");
                         goto success;
                     }
                     else if ( info_ack.fi_stat == FI_NEED_ALL)
                         process_indicator = PROCESS_SEND_BLOCK;
                     else if ( info_ack.fi_stat == FI_NEED_COMPARE)
                         process_indicator = PROCESS_SEND_VERIFY;
                     else {
                         MSG_DBG("ACK is FI_FAILED");
                         goto failed;
                     }
                     break;
                 }
                 case PROCESS_HANDLE_BLOCK_NEED: {
                     MSG_DBG("Turn into PROCESS_HANDLE_BLOCK_NEED (recv)");
                     if ( header.dh_tag != DHT_FILE_VB_ACK)
                         goto failed;
                     if ( header.dh_data_len == 0) {
                         MSG_DBG("Read a finish message");
                         read_flag = 0;
                         send_flag = 1;
                         // if ( send_flag == 0) {// 无文件块且文件块发送完毕时
                         //     goto success;
                         // }
                         break;
                     }
                     if ( recv_block_need() == false) {
                         // if ( tsd->message_empty_flag == 1 && send_flag == 0) {
                         //     goto success;
                         // }
                         goto failed;
                     } else {
                         send_flag = 1;
                     }
                     break;
                 }
                 default: {
                     MSG_DBG("Error state");
                     goto failed;
                 }
             }
        }


    }


success:

    if ( tsd->file_stream != NULL) {
        fclose(tsd->file_stream);
        tsd->file_stream = NULL;
    }
    if ( tsd->verify_stream != NULL) {
        fclose(tsd->verify_stream);
        tsd->verify_stream = NULL;
    }
    if ( tsd->send_list.list_len != 0) {
        block_no_node_t * node = tsd->send_list.head.next;
        while (node != NULL) {
            block_no_node_t * node_tmp = node;
            node = node->next;
            free(node_tmp);
        }
        tsd->send_list.list_len = 0;
        tsd->send_list.max_list_len = 0;
    }

    return true;


failed:
    deliver(tsd->sockfd, tsd->socktag, DHT_FILE_STOP_SEND, NULL, 0);

    if ( tsd->file_stream != NULL) {
        fclose(tsd->file_stream);
        tsd->file_stream = NULL;
    }
    if ( tsd->verify_stream != NULL) {
        fclose(tsd->verify_stream);
        tsd->verify_stream = NULL;
    }
    if ( tsd->send_list.list_len != 0) {
        block_no_node_t * node = tsd->send_list.head.next;
        while (node != NULL) {
            block_no_node_t * node_tmp = node;
            node = node->next;
            free(node_tmp);
        }
        tsd->send_list.list_len = 0;
        tsd->send_list.max_list_len = 0;
    }
    MSG_DBG_F("Send failed");
    return false;
}





static bool
send_info()
{
    tsd_t *              tsd = pthread_getspecific(tsd_key);
    file_info_t        filei;

    filei = (file_info_t){
         .fi_type            = FI_REGULAR,
         .fi_file_size       = htonll(tsd->file_stat.st_size),
         .fi_verify_size     = htonll(tsd->verify_stat.st_size),
         .fi_mode            = htonl(tsd->file_stat.st_mode),
         .fi_mtime_sec       = htonll(tsd->file_stat.st_mtimespec.tv_sec),
         .fi_block_len_set   = htonl(tsd->block_len_set),
         .fi_basepath_len    = htons(tsd->basepath_len)
     };

     memset(filei.fi_filepath, 0, sizeof(filei.fi_filepath));
     strncpy(filei.fi_filepath, tsd->file_path, sizeof(filei.fi_filepath));

     memset(filei.fi_basepath_hash, 0, sizeof(filei.fi_basepath_hash));
     strncpy(filei.fi_basepath_hash, tsd->basepath_hash, 64);

     if ( deliver(tsd->sockfd, tsd->socktag, DHT_FILE_INFORMATION, &filei, sizeof(filei)) == false) {
         goto failed;
     }

     return true;
 failed:
     return false;
}

static bool
get_info_ack(file_info_ack_t * ack)
{
    // file_info_ack_t     ack;
    tsd_t *             tsd = pthread_getspecific(tsd_key);

    // the ack only store a flag: DI_NEED_ALL, DI_NEED_COMPARE ...,
    // the type of flag is uint8_t
    if ( deliver_get_data(tsd->sockfd, tsd->socktag, ack, sizeof(*ack), &tsd->message_empty_flag) == false) {
        return false;
    }
    return true;
}


static bool
send_block(int block_no)
{
    tsd_t *                     tsd = pthread_getspecific(tsd_key);
    file_data_block_t           block;

    char *                      file_map_addr = NULL, * verify_map_addr = NULL;
    size_t                      file_map_len,           verify_map_len;
    struct iovec                iov[2];


    if ( block_no * tsd->block_len_set + tsd->block_len_set > tsd->file_stat.st_size) {
        file_map_len = tsd->file_stat.st_size - block_no * tsd->block_len_set;
    } else {
        file_map_len = tsd->block_len_set;
    }
    verify_map_len = tsd->verify_stat.st_size;

    // use mmap to deduce copy from kernel
    if ( (file_map_addr = mmap(NULL, file_map_len, PROT_READ, MAP_SHARED, fileno(tsd->file_stream), block_no * tsd->block_len_set)) == MAP_FAILED) {
        MSG_DBG_F("mmap failed: %s", strerror(errno));
        goto failed;
    }


    if ( (verify_map_addr = mmap(NULL, verify_map_len, PROT_READ, MAP_SHARED, fileno(tsd->verify_stream), 0)) == MAP_FAILED) {
        MSG_DBG_F("mmap failed: %s", strerror(errno));
        goto failed;
    }


    MSG_DBG_F("Send a block of file %s:\n\
               \tblock number is     %d,\n\
               \tblock length is     %d bytes\n",
                tsd->file_path, block_no, file_map_len);

    memcpy(block.fdb_data_hash, verify_map_addr+block_no*64, 64);
    block.fdb_no         = htonl(block_no);
    block.fdb_data_len   = htonl(file_map_len);


    iov[0].iov_base = &block;
    iov[0].iov_len = sizeof(block);
    iov[1].iov_base = file_map_addr;
    iov[1].iov_len = file_map_len;


    if ( deliver_scatter(tsd->sockfd, tsd->socktag, DHT_FILE_BLOCK, iov, 2) == false) {
        MSG_DBG_F("Write a block of \"%s\" failed, block number is %d ", tsd->file_path, block_no);
        goto failed;
    }


    if ( file_map_addr != NULL && file_map_addr != MAP_FAILED) munmap(file_map_addr, file_map_len);
    if ( verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, verify_map_len);
    return true;
failed:
    if ( file_map_addr != NULL && file_map_addr != MAP_FAILED) munmap(file_map_addr, file_map_len);
    if ( verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, verify_map_len);
    return false;

}


static bool
send_verify()
{
    tsd_t *                     tsd = pthread_getspecific(tsd_key);
    file_data_block_t           block;

    void *                      verify_map_addr = NULL;
    size_t                      map_len;

    MSG_DBG_F("Start send blocks of %s", tsd->verify_path);
    // rewind(tsd->file_stream);
    // rewind(tsd->verify_stream);
    if (tsd->verify_stat.st_size == 0) {
        MSG_DBG("verify_size is 0, don't need to send");
        goto success;
    }

    for (int verify_pos = 0; verify_pos < tsd->verify_stat.st_size; verify_pos += tsd->block_len_set) {
        if ( verify_pos + tsd->block_len_set > tsd->verify_stat.st_size) {
            map_len = tsd->verify_stat.st_size - verify_pos;
        } else {
            map_len = tsd->block_len_set;
        }

        block.fdb_no = htonl(verify_pos / tsd->block_len_set);
        block.fdb_data_len = htonl(map_len);

        // use mmap to deduce copy from kernel
        if ( (verify_map_addr = mmap(NULL, map_len, PROT_READ, MAP_PRIVATE, fileno(tsd->verify_stream), verify_pos)) == MAP_FAILED) {
            MSG_DBG_F("mmap failed: %s", strerror(errno));
            goto failed;
        }

        struct iovec iov[2];
        iov[0].iov_base = &block;
        iov[0].iov_len = sizeof(block);
        iov[1].iov_base = verify_map_addr;
        iov[1].iov_len = map_len;

        if ( deliver_scatter(tsd->sockfd, tsd->socktag, DHT_FILE_VERIFY_BLOCK, iov, 2) == false) {
            MSG_DBG("Deliver failed");
            goto failed;
        }

        munmap(verify_map_addr, map_len);
        verify_map_addr = NULL;
    }

success:
    if ( deliver(tsd->sockfd, tsd->socktag, DHT_FILE_VERIFY_BLOCK, NULL, 0) == false)
        return false;
    if ( verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, tsd->verify_stat.st_size);
    return true;
failed:
    if ( verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, tsd->verify_stat.st_size);
    return false;
}

static bool
recv_block_need()
{
    tsd_t *             tsd = pthread_getspecific(tsd_key);
    verify_block_ack_t  block_need;


    if ( deliver_get_data(tsd->sockfd, tsd->socktag, &block_need, sizeof(block_need), &tsd->message_empty_flag) == false)
        return false;
    else {
        block_need.vb_list_len = ntohl(block_need.vb_list_len);
        // pthread_mutex_lock(&tsd->send_list.mutex);
        block_no_node_t * node;
        for (int i = 0; i < block_need.vb_list_len; i++) {
            node = malloc(sizeof(block_no_node_t));
            node->number = ntohl(block_need.vb_list[i]);
            node->next = tsd->send_list.head.next;
            tsd->send_list.head.next = node;

            tsd->send_list.list_len++;
            tsd->send_list.max_list_len = tsd->send_list.list_len;
        }
        // pthread_mutex_unlock(&tsd->send_list.mutex);
    }
    return true;
}

static bool
send_block_need()
{
    tsd_t *             tsd = pthread_getspecific(tsd_key);
    block_no_node_t *   node;
    if ( tsd->send_list.head.next == NULL)
        return false;


    // pthread_mutex_lock(&tsd->send_list.mutex);
    node = tsd->send_list.head.next;
    tsd->send_list.head.next = node->next;
    tsd->send_list.list_len--;
    // pthread_mutex_unlock(&tsd->send_list.mutex);


    if ( send_block(node->number) == false) {
        free(node);
        return false;
    }

    free(node);
    return true;
}
