#include "check_error.hpp"
#include <cerrno>
#include <stdexcept>
#include <cstdio>
#include <cstring>

int check_common_error(int func_rtn, const char * func_name)
{
    if (func_rtn == -1) {
        char error_str[50];
        memset(error_str, 0, sizeof(error_str));
        snprintf(error_str, sizeof(error_str), "%s ==> %s (%d)", func_name, strerror(errno), errno);
        throw std::runtime_error(error_str);
    };
    return func_rtn;
}

int check_posix_thread_error(int func_rtn, const char * func_name)
{
    if (func_rtn != 0) {
        char error_str[50];
        memset(error_str, 0, sizeof(error_str));
        snprintf(error_str, sizeof(error_str), "%s ==> %s (%d)", func_name, strerror(func_rtn), errno);
        throw std::runtime_error(error_str);
    };
    return func_rtn;
}

int check_rpf_error(int func_rtn, const char * func_name)
{
    if (func_rtn != 0) {
        char error_str[50];
        memset(error_str, 0, sizeof(error_str));
        snprintf(error_str, sizeof(error_str), "%s ==> %s (%d)", func_name, strerror(errno), errno);
        throw std::runtime_error(error_str);
    };
    return func_rtn;
}
