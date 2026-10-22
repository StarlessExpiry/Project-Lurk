#include "file_deliver.h"
#include "deliver.h"
#include "../tools/makedir.h"
#include <stdbool.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>



static pthread_key_t    tsd_key;
static pthread_once_t   tsd_once_key = PTHREAD_ONCE_INIT;

typedef struct block_no_node {
    verify_block_ack_t       block_need_ack;
    struct block_no_node * next;
} block_need_node_t;

typedef struct {
    int              sockfd;
    sock_tag_t       socktag;
    size_t           message_len;
    int              message_empty_flag;
    uint32_t         block_len_set;

    // Absolute path to recv files
    const char *     dir_recv_file;
    // Absolute path to the verify_dir of recving your files
    const char *     dir_recv_verify;
    // recving file path
    char             filepath[4096];
    // e.g. current sending file is ~/send/your_file, the basepath is ~/send, length of it is 6
    uint16_t         basepath_len;
    // char             basepath_hash[65];
    // the verify path corresponding recving file path
    char             verifypath[4096];

    FILE *           file_stream, *      verify_stream;
    uint64_t         file_size,          verify_size;

    struct timespec  timespecs[2];                   /* [0] store accesstime, [1] store modifytime, all sec, ignore nsec*/

    block_need_node_t    need_list_head;
} tsd_t;

static void
tsd_destructor(void * ptr)
{
    tsd_t * tsd = ptr;
    if ( tsd->file_stream != NULL)
        fclose(tsd->file_stream);
    if ( tsd->verify_stream != NULL)
        fclose(tsd->verify_stream);
    free(ptr);
}

static void
tsd_once()
{
    pthread_key_create(&tsd_key, tsd_destructor);
}



static bool
recv_info(file_info_ack_t * info_ack);

static bool
send_info_ack(file_info_ack_t * info_ack);


static bool
recv_block();

static bool
recv_verify_and_compare();

static bool
send_block_need();



typedef enum {
    PROCESS_HANDLE_INFO,
    PROCESS_RECV_BLOCK, // 同时包含读写
    PROCESS_RECV_VERIFY_AND_COMPARE,
    PROCESS_HANDLE_BLOCK_NEED
} PROCESS_INDICATER_T;

typedef struct verify_ack_buff_node {
    verify_block_ack_t         hb_ack;
    struct verify_ack_buff_node *     next;
} verify_ack_buff_node_t;

bool
recv_single_file(int sockfd, sock_tag_t socktag, const char * path_to_recv, const char * verify_dir)
{
    int                 maxfd = sockfd;
    fd_set              rset, wset;
    int                 nready;
    tsd_t *             tsd;

    deliver_header_t    header;
    PROCESS_INDICATER_T process_indicator;


    MSG_DBG("Recving singfle file");

    memset(&header, 0, sizeof(header));


    pthread_once(&tsd_once_key, tsd_once);
    if ( (tsd = pthread_getspecific(tsd_key)) == NULL) {
        tsd = calloc(1, sizeof(tsd_t));
        pthread_setspecific(tsd_key, tsd);
    }

    tsd->sockfd = sockfd;
    tsd->socktag = socktag;


    make_dir(path_to_recv);
    make_dir(verify_dir);

    tsd->dir_recv_file = path_to_recv;
    tsd->dir_recv_verify = verify_dir;
    tsd->need_list_head.next = NULL;




    int     send_flag = 0;
    int     read_flag = 1;

    process_indicator = PROCESS_HANDLE_INFO;
    file_info_ack_t info_ack;
    while (1) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        // FD_SET(sockfd, &rset);
        if ( process_indicator == PROCESS_HANDLE_INFO && read_flag == 1)
            FD_SET(sockfd, &rset);
        if ( process_indicator == PROCESS_HANDLE_INFO && send_flag == 1)
            FD_SET(sockfd, &wset);
        if ( process_indicator == PROCESS_RECV_BLOCK)
            FD_SET(sockfd, &rset);
        if ( process_indicator == PROCESS_RECV_VERIFY_AND_COMPARE)
            FD_SET(sockfd, &rset);
        if ( process_indicator == PROCESS_HANDLE_BLOCK_NEED)
            FD_SET(sockfd, &rset);
        if ( process_indicator == PROCESS_HANDLE_BLOCK_NEED && send_flag == 1) {
            FD_SET(sockfd, &wset);
        }
        MSG_DBG("Select wait");
        if ( (nready = select(maxfd+1, &rset, &wset, NULL, NULL)) == -1) {
             if (errno == EINTR) {
                 continue;
             }else {
                 goto failed;
             }
        }


        if (FD_ISSET(sockfd, &rset)) {
            deliver_header_t header;
            int              empty_flag = 0;
            if ( deliver_get_header(tsd->sockfd, tsd->socktag, &header, &empty_flag) == false) {
                if (empty_flag == 1) {
                    MSG_DBG("One client exit");
                    goto success;
                } else {
                    MSG_DBG("Socket error");
                    goto failed;
                }
            } else {
                tsd->message_len = ntohll(header.dh_data_len);
            }
            if (header.dh_tag == DHT_FILE_STOP_SEND) {
                MSG_DBG("Client errror, stop recv");
                goto failed;
            }

            switch (process_indicator) {
                case PROCESS_HANDLE_INFO: {
                    MSG_DBG("Turn into PROCESS_HANDLE_INFO (recv)");
                    if (header.dh_tag != DHT_FILE_INFORMATION) {
                        goto failed;
                    }
                    memset(&info_ack, 0, sizeof(info_ack));
                    if ( recv_info(&info_ack) == false) {
                        MSG_DBG("Connot get file inform");
                        goto failed;
                    }
                    send_flag = 1;
                    read_flag = 0;
                    break;
                }
                case PROCESS_RECV_BLOCK: {
                    MSG_DBG("Turn into PROCESS_RECV_BLOCK");
                    if (header.dh_tag != DHT_FILE_BLOCK) {
                        goto failed;
                    }
                    if ( header.dh_data_len == 0) { // untill client send finish message
                        MSG_DBG("Recv done, success");
                        goto success;
                    }
                    if ( recv_block() == false) {
                        // if ( tsd->message_empty_flag == 1) {
                        //     goto success;
                        // } else {
                        goto failed;
                        // }
                    }
                    process_indicator = PROCESS_RECV_BLOCK; // loop

                    break;
                }
                case PROCESS_RECV_VERIFY_AND_COMPARE: {
                    MSG_DBG("Turn into PROCESS_RECV_VERIFY_AND_COMPARE");
                    if (header.dh_tag != DHT_FILE_VERIFY_BLOCK) {
                        goto failed;
                    }
                    if ( header.dh_data_len == 0) {
                        process_indicator = PROCESS_HANDLE_BLOCK_NEED;
                        send_flag = 1; // send need list first
                        break;
                    }
                    // continue receive blocks until client exit
                    // don't need to calculate block_recv_bytes
                    if ( recv_verify_and_compare() == false) {
                        // if ( tsd->message_empty_flag == 1) {
                        //     send_flag = 1;
                        //     process_indicator = PROCESS_HANDLE_BLOCK_NEED;
                        // } else {
                        //     goto failed;
                        // }
                        goto failed;
                    }
                    MSG_DBG("Recv a verify block");
                    break;
                }
                case PROCESS_HANDLE_BLOCK_NEED : {
                    MSG_DBG_F("Turn into PROCESS_HANDLE_BLOCK_NEED (recv)");
                    if (header.dh_tag != DHT_FILE_BLOCK) {
                        goto failed;
                    }
                    if ( header.dh_data_len == 0) {
                        MSG_DBG("Read all");
                        if ( send_flag == 0)
                            goto success;
                        break;
                    }
                    if ( recv_block() == false) {
                        // if ( tsd->message_empty_flag == 1) {
                        //     goto success;
                        // } else {
                        //     goto failed;
                        // }
                        goto failed;
                    }
                    break;

                }
            }
        }
        if ( FD_ISSET(sockfd, &wset)) {
            if ( process_indicator == PROCESS_HANDLE_INFO) {
                MSG_DBG("Turn into PROCESS_HANDLE_INFO (send)");
                if ( send_info_ack(&info_ack) == false) {
                    MSG_DBG_F("Send info_ack failed");
                    goto failed;
                }
                if ( info_ack.fi_stat == FI_FAILED) {
                    MSG_DBG_F("fi_stat is FI_FAILED");
                    goto failed;
                }
                else if ( info_ack.fi_stat == FI_NEED_COMPARE) {
                    process_indicator = PROCESS_RECV_VERIFY_AND_COMPARE;
                    MSG_DBG_F("fi_stat is FI_NEED_COMPARE");
                }
                else if (info_ack.fi_stat == FI_NEED_ALL) {
                    process_indicator = PROCESS_RECV_BLOCK;
                    MSG_DBG_F("fi_stat is FI_NEED_ALL");
                }
                else if (info_ack.fi_stat == FI_NO_NEED_DELIVER) {
                    MSG_DBG("No need deliver, quit");
                    goto success;
                }
                else {
                    MSG_DBG("info_ack.fi_stat read error");
                    goto failed;
                }
            }
            if ( process_indicator == PROCESS_HANDLE_BLOCK_NEED) {
                MSG_DBG("Turn into PROCESS_HANDLE_BLOCK_NEED (send)");
                if ( tsd->need_list_head.next == NULL) { // inform client that block_need_list all have been sent
                    MSG_DBG("Send all, donnot send any more");
                    if ( deliver(tsd->sockfd, tsd->socktag, DHT_FILE_VB_ACK, NULL, 0) == false)
                        goto failed;
                    send_flag = 0;
                    continue;
                }
                if ( send_block_need() == false) {
                    goto failed;
                }
            }
        }
    }


success:
    // fflush(tsd->file_stream);
    // fflush(tsd->verify_stream);
    // if (ftruncate(fileno(tsd->file_stream), tsd->file_size) != 0) {
    //     MSG_DBG_F("ftruncate failed, error is %s", strerror(errno));
    //     return false;
    // }
    // ftruncate(fileno(tsd->verify_stream), tsd->verify_size);
    // fclose(tsd->file_stream);
    // tsd->file_stream = NULL;
    // fclose(tsd->verify_stream);
    // tsd->verify_stream = NULL;

    if (tsd->file_stream != NULL) {
        fflush(tsd->file_stream);
        // if (ftruncate(fileno(tsd->file_stream), tsd->file_size) != 0) {
        //     MSG_DBG_F("ftruncate failed, error is %s", strerror(errno));
        //     return false;
        // }
        fclose(tsd->file_stream);
        tsd->file_stream = NULL;
    }
    if ( tsd->verify_stream != NULL) {
        fflush(tsd->verify_stream);
        // ftruncate(fileno(tsd->verify_stream), tsd->verify_size);
        fclose(tsd->verify_stream);
        tsd->verify_stream = NULL;
    }

    // 确保文件关闭后再设置访问时间(忽略)和修改时间
    if ( utimensat(AT_FDCWD, tsd->filepath, tsd->timespecs, AT_SYMLINK_NOFOLLOW) != 0) {
        MSG_DBG_F("Set modify time of file '%s' failed, error is %s", tsd->filepath, strerror(errno));
        return false;
    }

    if ( utimensat(AT_FDCWD, tsd->verifypath, tsd->timespecs, AT_SYMLINK_NOFOLLOW) != 0) {
        MSG_DBG_F("Set modify time of file '%s' failed, error is %s", tsd->verifypath, strerror(errno));
        return false;
    }

    MSG_DBG("Serve complete one client");

    return true;

failed:
    deliver(tsd->sockfd, tsd->socktag, DHT_FILE_STOP_SEND, NULL, 0);
    if (tsd->file_stream != NULL) {
        fflush(tsd->file_stream);
        // if (ftruncate(fileno(tsd->file_stream), tsd->file_size) != 0) {
        //     MSG_DBG_F("ftruncate failed, error is %s", strerror(errno));
        //     return false;
        // }
        fclose(tsd->file_stream);
        tsd->file_stream = NULL;
    }
    if ( tsd->verify_stream != NULL) {
        fflush(tsd->verify_stream);
        fclose(tsd->verify_stream);
        tsd->verify_stream = NULL;
    }

    MSG_DBG("RECV FAILED!");
    // if (ack_list_head.next != NULL) {
    //     ack_buff_node * p;
    //     while (ack_list_head.next == NULL) {
    //         p = ack_list_head.next;
    //         ack_list_head.next = p->next;
    //         free(p);
    //     }
    // }
    // if (filep_list[0] != NULL) {
    //     if (filep_list[0]->fp != NULL)  fclose(filep_list[0]->fp);
    //     free(filep_list[0]);
    // }
    // if (filep_list[1] != NULL) {
    //     if (filep_list[1]->fp != NULL)  fclose(filep_list[1]->fp);
    //     free(filep_list[1]);
    // }
    return false;
}


static bool
recv_info(file_info_ack_t * info_ack)
{
    tsd_t*              tsd = pthread_getspecific(tsd_key);
    file_info_t         inform_buf;

    memset(&inform_buf, 0, sizeof(inform_buf));
    MSG_DBG_F("message_len = %d", tsd->message_len);
    if ( deliver_get_data(tsd->sockfd, tsd->socktag, (char*)&inform_buf, tsd->message_len, &tsd->message_empty_flag) == false) {
        MSG_DBG_F("Connot read inform from peer");
        info_ack->fi_stat = FI_FAILED;
        goto failed;
    }




    inform_buf.fi_file_size     = ntohll(inform_buf.fi_file_size);
    inform_buf.fi_verify_size   = ntohll(inform_buf.fi_verify_size);
    inform_buf.fi_mode          = ntohl(inform_buf.fi_mode);
    inform_buf.fi_mtime_sec     = ntohll(inform_buf.fi_mtime_sec);
    inform_buf.fi_block_len_set = ntohl(inform_buf.fi_block_len_set);
    inform_buf.fi_basepath_len  = ntohs(inform_buf.fi_basepath_len);

    tsd->timespecs[0].tv_sec = 0;
    tsd->timespecs[0].tv_nsec = UTIME_OMIT;
    tsd->timespecs[1] = (struct timespec){(time_t)inform_buf.fi_mtime_sec, 0};
    tsd->block_len_set = inform_buf.fi_block_len_set;
    tsd->file_size = inform_buf.fi_file_size;
    tsd->verify_size = inform_buf.fi_verify_size;

    char filepath_except_basepath[4096];
    memset(filepath_except_basepath, 0, sizeof(filepath_except_basepath));
    strncpy(filepath_except_basepath, inform_buf.fi_filepath+inform_buf.fi_basepath_len+1, sizeof(filepath_except_basepath));


    memset(tsd->filepath, 0, sizeof(tsd->filepath));
    snprintf(tsd->filepath, sizeof(tsd->filepath)-1, "%s/%s/%s", tsd->dir_recv_file, inform_buf.fi_basepath_hash, filepath_except_basepath);
    tsd->basepath_len = inform_buf.fi_basepath_len;
    // memset(tsd->basepath_hash, 0, sizeof(tsd->basepath_hash));
    // strncpy(tsd->basepath_hash, inform_buf.fi_basepath_hash, sizeof(tsd->basepath_hash));

    memset(tsd->verifypath, 0, sizeof(tsd->verifypath));
    snprintf(tsd->verifypath, sizeof(tsd->verifypath), "%s/%s/%s", tsd->dir_recv_verify, inform_buf.fi_basepath_hash, filepath_except_basepath);

    make_dir(tsd->filepath);
    make_dir(tsd->verifypath);

    MSG_DBG_F("Receive info of file '%s'", inform_buf.fi_filepath);
    MSG_DBG_F("The correspoing file '%s'\n" \
              "\t\tverify file '%s'", tsd->filepath, tsd->verifypath);


    memset(info_ack, 0, sizeof(*info_ack));
    if (inform_buf.fi_type == FI_REGULAR) {
        if ( (tsd->verify_stream = fopen(tsd->verifypath, "r+")) == NULL) {
            if ( (tsd->verify_stream = fopen(tsd->verifypath, "w+")) == NULL) {
                MSG_DBG_F("Open verify file %s failed: %s", tsd->verifypath, strerror(errno));
                goto failed;
            }
        }
        MSG_DBG_F("file '%s' is regular file", tsd->filepath);
        if ( (tsd->file_stream = fopen(tsd->filepath, "rb+")) == NULL) {
            MSG_DBG_F("file '%s' do not exist in local, create it and recv all blocks", tsd->filepath);
            MSG_DBG_F("Now create file %s and verify file %s", tsd->filepath, tsd->verifypath);

            tsd->file_stream = fopen(tsd->filepath, "w+");
            if ( tsd->file_stream == NULL) {
                MSG_DBG_F("Create file %s failed, error is \"%s\"", tsd->filepath, strerror(errno));
                goto failed;
            }
            info_ack->fi_stat = FI_NEED_ALL;
            ftruncate(fileno(tsd->file_stream), inform_buf.fi_file_size);
            ftruncate(fileno(tsd->verify_stream), inform_buf.fi_verify_size);
        } else { // file has already exist
            struct stat file_stat;
            if ( stat(tsd->filepath, &file_stat) == -1)
                goto failed;
            if ( file_stat.st_mtimespec.tv_sec == inform_buf.fi_mtime_sec && file_stat.st_size == inform_buf.fi_file_size) {
                info_ack->fi_stat = FI_NO_NEED_DELIVER;
                MSG_DBG_F("File '%s' exist in local, and modify time and file size is match, do not need deliver", tsd->filepath);
            } else {
                info_ack->fi_stat = FI_NEED_COMPARE;
                MSG_DBG_F("%s exist in local, need compare the verify file", tsd->filepath);
                ftruncate(fileno(tsd->file_stream), inform_buf.fi_file_size);
                ftruncate(fileno(tsd->verify_stream), inform_buf.fi_verify_size);

                MSG_DBG_F("ftruncate file '%s' successfully, file_size = %ld", tsd->filepath, inform_buf.fi_file_size);
            }
        }
    } else {
        MSG_DBG_F("%s is not regular file", tsd->filepath);
        goto failed;
    }



    return true;
failed:
    info_ack->fi_stat = FI_FAILED;
    return false;
}

static bool
send_info_ack(file_info_ack_t * info_ack)
{
    tsd_t*              tsd = pthread_getspecific(tsd_key);
    MSG_DBG_F("Send info");
    if ( (deliver(tsd->sockfd, tsd->socktag, DHT_FILE_INFORMATION, (char *)info_ack, sizeof(*info_ack))) == false)
        return false;
    return true;
}




static bool
recv_block()
{
    tsd_t *                     tsd = pthread_getspecific(tsd_key);
    file_data_block_t           block;

    void *                      file_map_addr = NULL, *verify_map_addr = NULL;
    size_t                      file_map_len,   verify_map_len;

    if ( deliver_get_data(tsd->sockfd, tsd->socktag, (char*)&block, sizeof(block), &tsd->message_empty_flag) == false) {
        MSG_DBG("Recv error");
        goto failed;
    }


    block.fdb_no           = ntohl(block.fdb_no);
    block.fdb_data_len     = ntohl(block.fdb_data_len);
    // block->fdb_data_len_set = ntohl(block->fdb_data_len_set);
    MSG_DBG_F("Recv a block of file :\n\
               \tblock number is     %d,\n\
               \tblock length is     %d bytes\n",
                block.fdb_no, block.fdb_data_len);
    file_map_len = block.fdb_data_len;
    if ( block.fdb_no * tsd->block_len_set + tsd->block_len_set > tsd->file_size) {
        file_map_len = tsd->file_size - block.fdb_no * tsd->block_len_set;
    } else {
        file_map_len = tsd->block_len_set;
    }

    verify_map_len = tsd->verify_size; // because it is hard to Alignment (mmap)

    if ( (file_map_addr = mmap(NULL, file_map_len, PROT_WRITE, MAP_SHARED, fileno(tsd->file_stream), block.fdb_no * tsd->block_len_set)) == MAP_FAILED) {
        MSG_DBG_F("mmap failed: %s", strerror(errno));
        goto failed;
    }
    msync(file_map_addr, file_map_len, MS_SYNC);

    if ( (verify_map_addr = mmap(NULL, verify_map_len, PROT_WRITE, MAP_SHARED, fileno(tsd->verify_stream), 0)) == MAP_FAILED) {
        MSG_DBG_F("mmap failed: %s", strerror(errno));
        goto failed;
    }


    msync(verify_map_addr, verify_map_len, MS_SYNC);

    memcpy((char *)verify_map_addr+(block.fdb_no*64), block.fdb_data_hash, sizeof(block.fdb_data_hash));

    if ( deliver_get_data(tsd->sockfd, tsd->socktag, (char *)file_map_addr, block.fdb_data_len, &tsd->message_empty_flag) == false) {
        MSG_DBG("Recv error block");
        goto failed;
    }


    if (file_map_addr != NULL && file_map_addr != MAP_FAILED) munmap(file_map_addr, file_map_len);
    if (verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, verify_map_len);
    return true;
failed:
    if (file_map_addr != NULL && file_map_addr != MAP_FAILED) munmap(file_map_addr, file_map_len);
    if (verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, verify_map_len);
    return false;
}



static bool
recv_verify_and_compare()
{
    tsd_t *             tsd = pthread_getspecific(tsd_key);
    file_data_block_t * verify_data_block;
    block_need_node_t * block_need_node;
    // char                verify_data_to_compare[tsd->block_len_set];

    char *              verify_map_addr = NULL;
    size_t              map_len;


    MSG_DBG_F("tsd->message_len is %ld", tsd->message_len);
    if ( (verify_data_block = calloc(1, tsd->message_len)) == NULL) {
        goto failed;
    }
    if ( deliver_get_data(tsd->sockfd, tsd->socktag, (char*)verify_data_block, tsd->message_len, &tsd->message_empty_flag) == false)
        goto failed;

    verify_data_block->fdb_no = ntohl(verify_data_block->fdb_no);
    verify_data_block->fdb_data_len = ntohl(verify_data_block->fdb_data_len);


    if ( verify_data_block->fdb_no * tsd->block_len_set + tsd->block_len_set > tsd->verify_size) {
        map_len = tsd->verify_size - verify_data_block->fdb_no * tsd->block_len_set;
    } else {
        map_len = tsd->block_len_set;
    }

    if ( (verify_map_addr = mmap(NULL, map_len, PROT_WRITE, MAP_SHARED, fileno(tsd->verify_stream), verify_data_block->fdb_no * tsd->block_len_set)) == MAP_FAILED) {
        MSG_DBG_F("mmap failed: %s", strerror(errno));
        goto failed;
    }


    if ( (block_need_node = calloc(1, sizeof(block_need_node_t))) == NULL) {
        goto failed;
    }


    int insert_pos = 0;
    for (int i = 0; i < verify_data_block->fdb_data_len; i += 64) {
        if (strncmp(verify_map_addr+i, verify_data_block->fdb_data+i, 64) != 0) {
            MSG_DBG_F("block '%d' is not match，add it to block_need_list", verify_data_block->fdb_no+(i/64));
            block_need_node->block_need_ack.vb_list[insert_pos++] = htonl(verify_data_block->fdb_no + (i/64));
            block_need_node->block_need_ack.vb_list_len++;
        }else {
            MSG_DBG_F("block '%d' is match", verify_data_block->fdb_no + (i/64));
        }
    }
    if ( block_need_node->block_need_ack.vb_list_len != 0) {
        block_need_node->next = tsd->need_list_head.next;
        tsd->need_list_head.next = block_need_node;
        block_need_node->block_need_ack.vb_list_len = htonl(block_need_node->block_need_ack.vb_list_len);
    } else {
        MSG_DBG("block_need is zero, do not send");
        free(block_need_node);
    }

    if (verify_data_block != NULL) free(verify_data_block);
    if (verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, map_len);
    return true;
failed:
    if (verify_data_block != NULL) free(verify_data_block);
    if (verify_map_addr != NULL && verify_map_addr != MAP_FAILED) munmap(verify_map_addr, map_len);
    return false;
}

static bool
send_block_need()
{
    tsd_t *         tsd = pthread_getspecific(tsd_key);
    if ( tsd->need_list_head.next == NULL)
        return false;

    block_need_node_t * node;
    node = tsd->need_list_head.next;
    tsd->need_list_head.next = node->next;
    if ( deliver(tsd->sockfd, tsd->socktag, DHT_FILE_VB_ACK, &node->block_need_ack, sizeof(verify_block_ack_t)) == false)
        return false;
    free(node);

    return true;
}
