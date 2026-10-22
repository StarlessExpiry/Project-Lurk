#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


// bool init_config();
//
// uint16_t get_config_port();
//
// bool get_config_verifypath(char * verify_path);
//
// bool set_config_port(uint16_t port);
//
// bool set_config_verifypath(const char * verify_path);
//
// bool get_config_server_ip(char * server_ip_str);
//
// bool set_config_server_ip(const char * server_ip_str);
//
// uint16_t get_config_server_port();
//
// bool set_config_server_port(uint16_t port);


bool init_config_client();

bool init_config_server(char * recv_dir);

uint16_t get_config_deliver_file_port(int number);

bool get_config_verifypath_client(char * verify_path);

bool get_config_verifypath_server(char * verify_path);

bool get_config_recv_path_server(char * recv_path);

bool get_config_server_ip_client(char * server_ip_str);

bool get_config_server_ip_server(char * server_ip_str);

uint16_t get_config_server_port_client();

uint16_t get_config_server_port_server();

#ifdef __cplusplus
}
#endif
