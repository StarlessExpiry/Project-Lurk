#include "syserrno.h"
#include <errno.h>
#include <pthread.h>

#ifndef CONNPOOL_SIZE
#define CONNPOOL_SIZE 4
#endif

typedef struct {
    int connfd;
    int busy_flag; // is occupied or not
} single_connection_t;

typedef struct {
    single_connection_t pool[CONNPOOL_SIZE];
    int                 pool_busy_cnt;
    pthread_mutex_t     pool_mutex;
    pthread_cond_t      pool_cond;
} connection_pool_t;


// apply for a connfd by the connection_pool,
// if you can get, the funciton will return the connfd by the return of this function, that is "return connfd";
// if the conn_pool is error, return -1;
// otherwise, the function will block.
//
// Before pass vaiable "conn_pool" in, make sure that it has been initialized,
// including its mutex vaiable and cond vairable should all be initialized,
// and all of the descripters (connfd) should have already connected to peer.
//
// It is consumer.
int
connection_apply(connection_pool_t * const conn_pool);

int
connection_release(connection_pool_t * const conn_pool, int connfd_to_release);
