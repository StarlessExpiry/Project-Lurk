#include "msg_print.h"
#include "syserrno.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef MAXBUFLEN
#define MAXBUFLEN 125
#endif


extern int daemon_proc;

static char *
format_msg_dbg(const char * msg, const char * flag, const char * file, const char * func, const int line)
{
    char *          new_msg;
    // 获取当前终端的行数和列数
    struct winsize  w;
    // 当前终端的总宽度（列数)
    int             terminal_width = 0;


    ioctl(STDERR_FILENO, TIOCGWINSZ, &w);
    terminal_width = w.ws_col;
    // if cannot get terminal width, update it
    if ( terminal_width != 0) {
        int flag_len = strlen(flag);
        int suffix_len = (strlen(file) + strlen(func) + sizeof(line) + 2); // include 2 colons
        int space_len = terminal_width - suffix_len - flag_len;


        if (space_len <= strlen(file))
            space_len = strlen(file);

        if ( (new_msg = calloc(1, 2 * terminal_width)) == NULL)
            return NULL;

        snprintf(new_msg, 2 * terminal_width, "%*s%*s%s:%s:%d\n%10s%s\n", flag_len, flag, space_len, "\0", file, func, line, "\0", msg);
    } else {
        int new_msg_len = strlen(flag)+strlen(file)+strlen(func)+sizeof(line)+strlen(msg)+100;
        if ( (new_msg = calloc(1, new_msg_len)) == NULL)
            return NULL;

        snprintf(new_msg, new_msg_len, "%s(%s:%s:%d)\n%10s%s\n", flag, file, func, line, "\0", msg);
    }


    return new_msg;
}

static char *
format_msg(const char * msg, const char * flag)
{
    char *      new_msg;

    if ( (new_msg = malloc(strlen(msg) + strlen(flag) + 2)) == NULL)
        return NULL;

    // 格式化内容
    sprintf(new_msg, "%-10s%s\n", flag, msg);
    return new_msg;
}

static void
msg_print(FILE * io, int level, char * new_msg, va_list vl)
{
    // char buf[MAXBUFLEN] = {};
    // vsnprintf(buf, MAXBUFLEN, new_msg, vl);
    if (daemon_proc == 1) // for daemon
        vsyslog(level, new_msg, vl);
    else {
        if (vl == NULL) {
            fputs(new_msg, io);
        }else
            vfprintf(io, new_msg, vl);
        if (io == stderr) {
            // do nothing
        } else if (io == stdout) {
            fflush(io);
        } else { 
            fflush(io);
        }
    }
    free(new_msg);
}



void msg_wrn(const char *msg)
{
    char *new_msg = format_msg(msg, "[Warning] ");
    msg_print(stderr, LOG_WARNING, new_msg, NULL);
}

void msg_dbg(const char *file, const char * func, const int line, const char *msg)
{
    char *new_msg = format_msg_dbg(msg, "[Debug]   ", file, func, line);
    msg_print(stderr, LOG_DEBUG, new_msg, NULL);
}

void msg_err(const char *msg)
{
    char *new_msg = format_msg(msg, "[Error]   ");
    msg_print(stderr, LOG_ERR, new_msg, NULL);
    exit(EXIT_FAILURE);
}

void msg_info(const char *msg)
{
    char *new_msg = format_msg(msg, "[INFO]    ");
    msg_print(stderr, LOG_NOTICE, new_msg, NULL);
}



void msg_wrn_f(const char *msg, ...)
{
    char *new_msg = format_msg(msg, "[Warning] ");
    va_list args;
    va_start(args, msg);
    msg_print(stderr, LOG_WARNING, new_msg, args);
    va_end(args);
}

void msg_dbg_f(const char *file, const char * func, const int line, const char *msg, ...)
{
    char *new_msg = format_msg_dbg(msg, "[Debug]   ", file, func, line);
    va_list args;
    va_start(args, msg);
    msg_print(stderr, LOG_DEBUG, new_msg, args);
    va_end(args);
}

void msg_err_f(const char *msg, ...)
{
    char *new_msg = format_msg(msg, "[Error]   ");
    va_list args;
    va_start(args, msg);
    msg_print(stderr, LOG_ERR, new_msg, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void msg_info_f(const char *msg, ...)
{
    char *new_msg = format_msg(msg, "[INFO]    ");
    va_list args;
    va_start(args, msg);
    msg_print(stderr, LOG_NOTICE, new_msg, args);
    va_end(args);
}

void
msg_print_error_dbg(const char * file, const char * func, const int line)
{
    char    buf_tmp[1024];
    char *  msg;

    strsyserror(syserrno, buf_tmp, sizeof(buf_tmp));
    msg = format_msg_dbg(buf_tmp, "[Debug]   ", file, func, line);
    msg_print(stderr, LOG_DEBUG, msg, NULL);
}

void
msg_print_error()
{
    char    buf_tmp[1024];
    char *  msg;

    strsyserror(syserrno, buf_tmp, sizeof(buf_tmp));
    msg = format_msg(buf_tmp, "[Error]   ");
    msg_print(stderr, LOG_ERR, msg, NULL);
}
