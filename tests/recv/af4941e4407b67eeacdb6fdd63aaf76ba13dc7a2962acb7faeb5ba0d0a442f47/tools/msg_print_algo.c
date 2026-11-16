#include "msg_print_algo.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAXBUFLEN_ALGO
#define MAXBUFLEN_ALGO 125
#endif /* ifndef MAXBUFLEN */


extern int daemon_proc;

// 字符串添加信息
char * format_msg(const char * msg, const char * flag)
{
    char *new_msg = malloc(strlen(msg) + strlen(flag) + 2);
    if (!new_msg) return NULL;
    sprintf(new_msg, "%s%s\n", flag, msg);
    return new_msg;
}

// msg_print.c中的私有函数, 用于根据daemon_proc标志进行不同类型的输出,
// 是vsyslog还是fputs还是vfprintf
void
msg_print(FILE * io, int level, char * new_msg, va_list vl)
{
    // char buf[MAXBUFLEN] = {};
    // vsnprintf(buf, MAXBUFLEN, new_msg, vl);
    if (daemon_proc == 1) // 作为守护进程进行输出
        vsyslog(level, new_msg, vl);
    else {
        if (vl == NULL) {
            fputs(new_msg, io);
        }else
            vfprintf(io, new_msg, vl);
        if (io == stderr) { // 不缓冲
            // do nothing
        } else if (io == stdout) { // 完全缓冲
            fflush(io);
        } else { // 其他io流都是完全缓冲
            fflush(io);
        }
    }
    free(new_msg);
}



void
msg_dbg(const char *msg)
{
#ifdef DEBUG
    char * new_msg = format_msg(msg, "[Debug]   ");
    msg_print(stdout, LOG_DEBUG, new_msg, NULL);
#endif /* ifdef DEBUG */
}





void
msg_dbg_f(const char *msg, ...) 
{
#ifdef DEBUG
    char * new_msg = format_msg(msg, "[Debug]   ");
    va_list args;
    va_start(args, msg);
    msg_print(stderr, LOG_DEBUG, new_msg, args);
    va_end(args);
#endif /* ifdef DEBUG */
}


