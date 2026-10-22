#include "io.h"
#include "msg_print.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_key_t  rl_key;
static pthread_once_t rl_once = PTHREAD_ONCE_INIT;


static void
readline_destructor(void *ptr)
{
    free(ptr);
}




/**
 * @brief   从描述符指定缓冲区中读取内容（阻塞），按照read_require读取，
 *          要么读完read_require个字节，要么把描述符缓冲区读空;
 */
int
read_all(int fd, char * buf, int32_t read_require)
{
    char *      buf_ptr = buf;
    int         read_cnt;
    uint32_t    left_cnt = read_require;

    while (left_cnt > 0) {
        if ( (read_cnt = read(fd, buf_ptr, left_cnt)) < 0) {
            if (errno == EINTR){ // 被信号中断
                // left_cnt = read_len;    // 应重新调用 read
                continue;
            }
            else // 错误
                return -1;
        } else if (read_cnt == 0) { // 缓冲区为空，或者对端关闭
            MSG_DBG("eof. read done");
            break;
        }

        left_cnt -= read_cnt;
        buf_ptr += read_cnt; // 移动文件指针ptr，接着进行写入，使得数据连续
    }

    MSG_DBG_F("Read done. require %d, left = %d (0 is OK)", read_require, left_cnt);
    return read_require - left_cnt; // read_require作为值结果参数，返回读到多少
}



int
write_all(int fd, char * restrict buf, int32_t write_require)
{
    char *  buf_ptr = buf;
    int     write_cnt;
    int     left_cnt = write_require;

    while (left_cnt > 0) {
        if ( (write_cnt = write(fd, buf_ptr, left_cnt)) < 0) {
            if (errno == EINTR){ // 被信号中断
                // left_cnt = read_len;    // 应重新调用 write
                continue;
            }
            else // 错误
                return -1;
        }

        left_cnt -= write_cnt;
        buf_ptr += write_cnt;
    }
    MSG_DBG_F("Require %d, left = %d (0 is ok)", write_require, left_cnt);
    return write_require - left_cnt; // 返回写了多少
}

int
read_stream_all(FILE * stream, char * buf, int32_t read_require)
{
    char *      buf_ptr = buf;
    int         read_cnt;
    uint32_t    left_cnt = read_require;

    while (left_cnt > 0) {
        if ( (read_cnt = fread(buf_ptr, 1, left_cnt, stream)) < 0) {
            if (errno == EINTR){ // 被信号中断
                // left_cnt = read_len;    // 应重新调用 read
                continue;
            }
            else // 错误
                return -1;
        } else if (read_cnt == 0) { // 缓冲区为空，或者对端关闭
            MSG_DBG("eof. read done");
            break;
        }

        left_cnt -= read_cnt;
        buf_ptr += read_cnt; // 移动文件指针ptr，接着进行写入，使得数据连续
    }

    MSG_DBG_F("Read done. require %d, left = %d (0 is OK)", read_require, left_cnt);
    return read_require - left_cnt; // read_require作为值结果参数，返回读到多少
}

int
write_stream_all(FILE * stream, char * restrict buf, int32_t write_require)
{

    char *  buf_ptr = buf;
    int     write_cnt;
    int     left_cnt = write_require;

    while (left_cnt > 0) {
        if ( (write_cnt = fwrite(buf_ptr, 1, left_cnt, stream)) < 0) {
            if (errno == EINTR){ // 被信号中断
                // left_cnt = read_len;    // 应重新调用 write
                continue;
            }
            else // 错误
                return -1;
        }

        left_cnt -= write_cnt;
        buf_ptr += write_cnt;
    }
    MSG_DBG_F("Require %d, left = %d (0 is ok)", write_require, left_cnt);
    return write_require - left_cnt; // 返回写了多少
}


ssize_t
read_byte(read_buf_info *buf, int fd, char *rtn_byte)
{
    // 无待读数据, 从对应描述符缓冲区中取数据, 放进buf中;
    // 有待读数据, 从buf中取数据;
    if (buf->rb_bytes_to_read <= 0) {
    again:
        // 尽力从缓冲区中取数据
        if ((buf->rb_bytes_to_read = read(fd, buf->rb_buf, REALINE_MAXLINE)) < 0) {
            if (errno == EINTR)
                goto again;
            return (-1);
        } else if (buf->rb_bytes_to_read == 0) // 没读到就返回
            return (0);
        buf->rb_input_ptr = buf->rb_buf;
        return (0);
    }else {
        buf->rb_bytes_to_read--;
        *rtn_byte = *buf->rb_input_ptr++;
        return (1);
    }
}


// ssize_t
// readline_thread(int fd, void *buf_ptr, size_t maxlen)
// {
//     // 创建Key[i]
//     Pthread_once(&rl_once, readline_once);

//     // 创建pkey[i]线程特定数据
//     read_buf_info *buf;
//     if ((buf = pthread_getspecific(rl_key)) == NULL) {
//         buf = calloc(1, sizeof(read_buf_info));
//         Pthread_setspecific(rl_key, buf);
//     }

//     char * ptr = buf_ptr;
//     char rtn_byte;

//     size_t n, read_cnt;
//     for (n = 1; n < maxlen; n++) {
//         if ((read_cnt = read_byte(buf, fd, &rtn_byte)) == 1) {
//             *ptr++ = rtn_byte;
//             if (rtn_byte == '\n')
//                 break;
//         } else if (read_cnt == 0) {// EOF, 返回已读的字节数n-1
//             *ptr = '\0';
//             return (n - 1);
//         } else // 没有读到任何数据, 返回错误
//             return (-1);
//     }

//     *ptr = '\0';
//     return (n);
// }

// ssize_t
// Readline_thread(int fd, void *ptr, size_t maxlen)
// {
//     ssize_t n;

//     if ((n = readline_thread(fd, ptr, maxlen)) < 0)
//         msg_err("Readline error");
//     return (n);
// }
