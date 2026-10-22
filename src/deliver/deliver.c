#include "deliver.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>


// typedef struct {
//     int sockfd;
//     int socktag;
// } tsd_t;
// 
// 
// static pthread_key_t    tsd_key;
// static pthread_once_t   tsd_once_key = PTHREAD_ONCE_INIT;
// 
// 
// static void
// tsd_destructor(void * ptr)
// {
//     free(ptr);
// }
// 
// static void
// tsd_once()
// {
//     pthread_key_create(&tsd_key, tsd_destructor);
// }
// 
// bool
// deliver_config(int sockfd, sock_tag_t socktag) {
//     pthread_once(&tsd_once_key, tsd_once);
//     tsd_t * tsd;
//     if ( (tsd = pthread_getspecific(tsd_key)) == NULL) {
//         tsd = calloc(1, sizeof(tsd_t));
//         pthread_setspecific(tsd_key, tsd);
//     }
//     tsd->sockfd = sockfd;
//     tsd->socktag = socktag;
//     return true;
// }


bool
deliver(int sockfd, sock_tag_t socktag, int header_tag, const void * deliver_data, size_t deliver_data_len) {
    // tsd_t *                     tsd = pthread_getspecific(tsd_key);
    deliver_header_t            deliver_hdr;
    struct iovec                iov[2];
    struct msghdr               msgh;
    // char *                      deliver_buff = calloc(1, sizeof(deliver_hdr)+deliver_data_len);
    // int                         send_cnt;


    // if ( deliver_buff == NULL) {
    //     goto failed;
    // }

    deliver_hdr = (deliver_header_t) {
        header_tag,
        htonll(deliver_data_len)
    };


    iov[0].iov_base = &deliver_hdr;
    iov[0].iov_len = sizeof(deliver_hdr);
    iov[1].iov_base = (void *)deliver_data;
    iov[1].iov_len = deliver_data_len;

    memset(&msgh, 0, sizeof(struct msghdr));
    if ( deliver_data == NULL || deliver_data_len == 0) {
        msgh.msg_name = NULL;
        msgh.msg_namelen = 0;
        msgh.msg_iov = iov;
        msgh.msg_iovlen = 1;
        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;
    } else {
        msgh.msg_name = NULL;
        msgh.msg_namelen = 0;
        msgh.msg_iov = iov;
        msgh.msg_iovlen = 2;
        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;
    }

    // memcpy(deliver_buff, &deliver_hdr, sizeof(deliver_hdr));
    // if ( deliver_data != NULL) {
    //     memcpy(deliver_buff+sizeof(deliver_hdr), deliver_data, deliver_data_len);
    // }

    if ( socktag == ST_LAN) {
        // int send_bytes = 0;
        int cur_bytes;
        // while ( send_bytes < sizeof(deliver_hdr)+deliver_data_len) {
            // if ( (cur_bytes = send(tsd->sockfd, deliver_buff+send_bytes, sizeof(deliver_hdr)+deliver_data_len-send_bytes, MSG_DONTROUTE)) == -1)
            //     goto failed;
        if ( (cur_bytes = sendmsg(sockfd, &msgh, MSG_DONTROUTE)) == -1) {
            MSG_DBG_F("Deliver failed: %s", strerror(errno));
            goto failed;
        }
            // send_bytes += cur_bytes;
        // }
    } else if (socktag == ST_COMMON) {
        // int send_bytes = 0;
        int cur_bytes;
        // while ( send_bytes < sizeof(deliver_hdr)+deliver_data_len) {
            // if ( (cur_bytes = send(tsd->sockfd, deliver_buff+send_bytes, sizeof(deliver_hdr)+deliver_data_len-send_bytes, 0)) == -1)
            //     goto failed;
            if ( (cur_bytes = sendmsg(sockfd, &msgh, 0)) == -1) {
                MSG_DBG_F("Deliver failed: %s", strerror(errno));
                goto failed;
            }
        //     send_bytes += cur_bytes;
        // }
        // send(tsd->sockfd, deliver_buff, sizeof(deliver_hdr)+deliver_data_len, 0);
    } else if (socktag == ST_UNIX) {

    }

    // if (deliver_buff != NULL) free(deliver_buff);
    return true;
failed:
    // if (deliver_buff != NULL) free(deliver_buff);
    return false;
}
bool deliver_scatter(int sockfd, sock_tag_t socktag, int header_tag, const struct iovec * data_iov, int data_iov_cnt) {
    deliver_header_t            deliver_hdr;
    // char *                      deliver_buff = calloc(1, sizeof(deliver_hdr)+deliver_data_len);
    // int                         send_cnt;
    int                         iov_bytes;
    struct msghdr               this_msghdr;


    struct iovec                iov[data_iov_cnt+1];




    // if ( deliver_buff == NULL) {
    //     goto failed;
    // }


    iov_bytes = 0;
    for (int i = 0; i < data_iov_cnt; i++) {
        iov_bytes += data_iov[i].iov_len;
    }

    deliver_hdr = (deliver_header_t) {
        header_tag,
        ntohll(iov_bytes)
    };

    iov[0].iov_base = &deliver_hdr;
    iov[0].iov_len = sizeof(deliver_hdr);

    for (int i = 0; i < data_iov_cnt; i++) {
        iov[i+1].iov_base = data_iov[i].iov_base;
        iov[i+1].iov_len = data_iov[i].iov_len;
    }
    memset(&this_msghdr, 0, sizeof(this_msghdr));

    this_msghdr.msg_name = NULL;
    this_msghdr.msg_namelen = 0;
    this_msghdr.msg_iov = iov;
    this_msghdr.msg_iovlen = data_iov_cnt+1;
    this_msghdr.msg_control = NULL;
    this_msghdr.msg_controllen = 0;
    this_msghdr.msg_flags = 0;



    if (socktag == ST_LAN) {
        // int send_bytes = 0;
        // int cur_bytes;

        if ( sendmsg(sockfd, &this_msghdr, MSG_DONTROUTE) == -1) {
            MSG_DBG_F("sendmsg error: %s", strerror(errno));
            goto failed;
        }


        // while ( send_bytes < sizeof(deliver_hdr)+deliver_data_len) {
        //     if ( (cur_bytes = send(tsd->sockfd, deliver_buff+send_bytes, sizeof(deliver_hdr)+deliver_data_len-send_bytes, MSG_DONTROUTE)) == -1)
        //         goto failed;
        //     send_bytes += cur_bytes;
        // }
    } else if (socktag == ST_COMMON) {
        // int send_bytes = 0;
        // int cur_bytes;

        if ( sendmsg(sockfd, &this_msghdr, 0) == -1) {
            MSG_DBG_F("sendmsg error: %s", strerror(errno));
            goto failed;
        }
        // while ( send_bytes < sizeof(deliver_hdr)+deliver_data_len) {
        //     if ( (cur_bytes = send(tsd->sockfd, deliver_buff+send_bytes, sizeof(deliver_hdr)+deliver_data_len-send_bytes, 0)) == -1)
        //         goto failed;
        //     send_bytes += cur_bytes;
        // }
        // send(tsd->sockfd, deliver_buff, sizeof(deliver_hdr)+deliver_data_len, 0);
    } else if (socktag == ST_UNIX) {

    }

    // if (deliver_buff != NULL) free(deliver_buff);
    return true;
failed:
    // if (deliver_buff != NULL) free(deliver_buff);
    return false;

}

bool
deliver_peek_header(int sockfd, sock_tag_t socktag, deliver_header_t * header,int * empty_flag)
{
    int recv_cnt = 0;
    if (socktag == ST_UNIX) {

    } else {
        recv_cnt = recv(sockfd, header, sizeof(*header), MSG_PEEK);
    }

    if ( recv_cnt == -1) {
        *empty_flag = 0;
        return false;
    }
    if ( recv_cnt == 0) {
        *empty_flag = 1;
        return false;
    }

    *empty_flag = 0;
    return true;
}

bool
deliver_get_header(int sockfd, sock_tag_t socktag, deliver_header_t * header, int * empty_flag)
{
    int recv_bytes = 0;
    int cur_bytes;
    if (socktag == ST_UNIX) {

    } else {
        while (recv_bytes < sizeof(*header)) {
            if ( (cur_bytes = recv(sockfd, (char*)header+recv_bytes, sizeof(*header)-recv_bytes, 0)) == -1) {
                *empty_flag = 0;
                return false;
            } else if (cur_bytes == 0) {
                *empty_flag = 1;
                return false;
            }
            recv_bytes += cur_bytes;
        }
    }
    return true;
}

bool
deliver_get_data(int sockfd, sock_tag_t socktag, void * data, int data_len, int * empty_flag)
{
    int recv_bytes = 0;
    int cur_bytes = 0;
    if (socktag == ST_UNIX) {

    } else {
        // recv_cnt = recv(tsd->sockfd, data, data_len, 0);
        while (recv_bytes < data_len) {
            if ( (cur_bytes = recv(sockfd, data+recv_bytes, data_len-recv_bytes, 0)) == -1) {
                MSG_DBG_F("recv failed: %s", strerror(errno));
                *empty_flag = 0;
                return false;
            } else if (cur_bytes == 0) {
                *empty_flag = 1;
                return false;
            }
            recv_bytes += cur_bytes;
        }
    }

    // if ( recv_cnt == -1) {
    //     *empty_flag = 1;
    //     return false;
    // }
    // if ( recv_cnt == 0) {
    //     *empty_flag = 1;
    //     return false;
    // }
    return true;
}
