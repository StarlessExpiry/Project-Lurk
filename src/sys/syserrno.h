#pragma once


#define E_SEND_BLOCK            1001
#define E_PARAMETER_MISMATCH    1002
#define E_MEMALLOCATE_FAILED    1003

__attribute__((visibility("hidden")))
int *
syserror();

#define syserrno (*syserror())


void
strsyserror(int errnum, char buf[], int buf_len);

// gatherify
typedef enum {
    GE_ALL_DONE = 0,
    GE_PARAMETER_MISMATCH = 1,
    GE_MEMALLOCATE_FAILED,
} gfy_stat_t;
