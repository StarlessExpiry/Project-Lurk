#pragma once
#include <cstdint>
#include <unistd.h>



void init_config_client();

void init_config_server(const char * recv_path);

void get_config_recv_path_server(char recv_path[], size_t recv_path_len);

uint16_t get_config_deliver_file_port(int number);

void get_config_verifypath(char verify_path[], size_t verify_path_len);

void get_config_server_ip(char server_ip_str[], size_t server_ip_str_len);

uint16_t get_config_server_port();
