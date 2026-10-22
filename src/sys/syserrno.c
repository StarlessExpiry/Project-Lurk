#include "syserrno.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static pthread_key_t    syserrno_key;
static pthread_once_t   syserrno_once = PTHREAD_ONCE_INIT;

// 系统自动调用
static void
syserror_destructor(void * ptr) {
    free(ptr);
}

static void
syserror_once() {
    pthread_key_create(&syserrno_key, syserror_destructor);
}

int *
syserror() {
    int * syserrno_ptr;
    
    pthread_once(&syserrno_once, syserror_once);
    if ( (syserrno_ptr = pthread_getspecific(syserrno_key)) == NULL) {
        syserrno_ptr = malloc(sizeof(int));
        pthread_setspecific(syserrno_key, syserrno_ptr);
    }

    return syserrno_ptr;
}

void
strsyserror(int errnum, char buf[], int buf_len) {
    if (errnum <= 106 && errnum >= 0) {
        strerror_r(errnum, buf, buf_len);
        return;
    }
    switch (errnum) {
    }
}


