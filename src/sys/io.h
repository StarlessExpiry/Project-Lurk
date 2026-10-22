#pragma once
#include <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif

#define REALINE_MAXLINE 4096

typedef struct {
    char  rb_buf[REALINE_MAXLINE];
    int   rb_bytes_to_read;    // 缓冲区中待读的字符数目
    char *rb_input_ptr;
} read_buf_info;


// 函数:    从描述符fd(建议是文件)中尽力读取数据, 直到eof;
// 参数:    int fd          描述符
//          char * buf      将数据存进的缓冲区
//          int read_len    要求读取的字节数
// 返回值:  >=0, 返回读到的字节数;
//          错误返回-1, 设置errno;
int
read_all(int fd, char * buf, int32_t read_len);

int
write_all(int fd, char * buf, int32_t write_len);

// 函数:    返回描述符缓冲区中的数据, 每次返回1字节, 支持连续调用
// 参数:    read_buf *buf   传入暂存数据的缓冲区（非全局变量）
//          int fd          待读的描述符
//          char * rtn_byte 值-结果参数, 传回的字节
// 返回值:  ssize_t         和read类似, 读到字节返回1, 没读到返回0, 出错返回-1
ssize_t
read_byte(read_buf_info *buf, int fd, char *rtn_byte);


ssize_t
readline_thread(int fd, void *buf_ptr, size_t maxlen);

ssize_t
Readline_thread(int fd, void *ptr, size_t maxlen);

#ifdef __cplusplus
}
#endif