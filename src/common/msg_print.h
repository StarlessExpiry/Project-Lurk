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
