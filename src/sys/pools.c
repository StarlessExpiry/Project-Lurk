#include "pools.h"

int
connection_apply(connection_pool_t * const conn_pool) {
    int         connfd;

    if (conn_pool == NULL) {
        syserrno = E_PARAMETER_MISMATCH;
        return (-1);
    }

    if ( (syserrno = pthread_mutex_lock(&conn_pool->pool_mutex)) > 0) {
        return (-1);
    }
    while (conn_pool->pool_busy_cnt >= CONNPOOL_SIZE) {
        pthread_cond_wait(&conn_pool->pool_cond, &conn_pool->pool_mutex);
    }
    conn_pool->pool_busy_cnt++;
    for (int i = 0; i < CONNPOOL_SIZE; i++) {
        if ( conn_pool->pool[i].busy_flag == 0) {
            connfd = conn_pool->pool[i].connfd;
            conn_pool->pool[i].busy_flag = 1;
            break;
        }
    }
    if ( (syserrno = pthread_mutex_unlock(&conn_pool->pool_mutex)) > 0) {
        return (-1);
    }

    return connfd;
}




int
connection_release(connection_pool_t * const conn_pool, int connfd_to_release) {
    int         signal_flag = 0;
    if (conn_pool == NULL) {
        syserrno = E_PARAMETER_MISMATCH;
        return (-1);
    }

    if ( (syserrno = pthread_mutex_lock(&conn_pool->pool_mutex)) > 0) {
        return (-1);
    }
    conn_pool->pool_busy_cnt--;
    for (int i = 0; i < CONNPOOL_SIZE; i++) {
        if ( conn_pool->pool[i].connfd == connfd_to_release) {
            conn_pool->pool[i].busy_flag = 0;
            signal_flag = 1;
            break;
        }
    }
    if ( (syserrno = pthread_mutex_unlock(&conn_pool->pool_mutex)) > 0) {
        return (-1);
    }

    if ( signal_flag == 1) {
        pthread_cond_broadcast(&conn_pool->pool_cond);
    }

    return (0);
}


