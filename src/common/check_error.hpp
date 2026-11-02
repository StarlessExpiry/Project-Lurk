#pragma once

bool check_common_error(bool func_rtn, const char * error_msg);

int check_syscall_error(int func_rtn, const char * func_name);

int check_posix_thread_error(int func_rtn, const char * func_name);

int check_rpf_error(int func_rtn, const char * func_name);
