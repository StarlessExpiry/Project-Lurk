#pragma once


#ifdef __cplusplus
extern "C" {
#endif

void
msg_wrn(const char * msg);
void
msg_dbg(const char * file, const char * func, const int line, const char * msg);
void
msg_err(const char * msg);
void
msg_info(const char * msg);


void
msg_wrn_f(const char * msg, ...);
void
msg_dbg_f(const char * file, const char * func, const int line, const char *msg, ...);
// 说明: 使用stderr输出格式化后的错误信息, 输出完成后不退出（应交给调用者处理）;
// 注意: 若结合perror使用, 应该在perror后调用;
void
msg_err_f(const char *msg, ...);
void
msg_info_f(const char *msg, ...);

void
msg_print_error_dbg(const char * file, const char * func, const int line);

void
msg_print_error();



#ifdef DEBUG
#define MSG_DBG(msg) \
    msg_dbg(__FILE__,  __func__, __LINE__, (msg))


#define MSG_DBG_F(msg, ...) \
    msg_dbg_f(__FILE__, __func__, __LINE__, (msg), ##__VA_ARGS__)


#define MSG_PRINT_ERROR_DBG() \
    msg_print_error_dbg(__FILE__, __func__, __LINE__)
#else
#define MSG_DBG(msg) 
#define MSG_DBG_F(msg, ...)
#define MSG_PRINT_ERROR_DBG()
#endif



#ifdef __cplusplus
}
#endif
