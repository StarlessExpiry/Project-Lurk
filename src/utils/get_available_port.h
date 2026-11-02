#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <netdb.h>

bool get_available_ip(char ip_str[NI_MAXHOST]);

uint16_t get_available_port();


#ifdef __cplusplus
}
#endif
