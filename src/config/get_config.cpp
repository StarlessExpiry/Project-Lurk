#include "config/get_config.hpp"
#include "common/check_error.hpp"
#include "utils/get_available_port.h"
#include "utils/makedir.h"
#include "common/msg_print.h"
#include "yaml-cpp/yaml.h"
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sys/stat.h>


typedef struct {
    char        yaml_path[4096];
    char        verify_path[4096];
    YAML::Node  config;
} tsd_t;

static pthread_key_t    tsd_key;
static pthread_once_t   tsd_once_key  = PTHREAD_ONCE_INIT;

static void
tsd_destructor(void * ptr)
{
    delete (tsd_t *)ptr;
}

static void
tsd_once()
{
    pthread_key_create(&tsd_key, tsd_destructor);
}

// should do nothing, just get from server config file
void init_config_client()
{

    pthread_once(&tsd_once_key, tsd_once);
    tsd_t * tsd = new tsd_t;
    pthread_setspecific(tsd_key, tsd);

    const char *   home_path = getenv("HOME");
    // char            yaml_path[4096];
    memset(tsd->yaml_path, 0, sizeof(tsd->yaml_path));
    snprintf(tsd->yaml_path, sizeof(tsd->yaml_path), "%s/%s", home_path, ".config/lurk/lurk_client_config.yaml");
    make_dir(tsd->yaml_path);

    memset(tsd->verify_path, 0, sizeof(tsd->verify_path));
    snprintf(tsd->verify_path, sizeof(tsd->verify_path), "%s/%s", home_path, ".cache/lurk/client");
    tsd->verify_path[strlen(tsd->verify_path)] = '/';
    make_dir(tsd->verify_path);
    tsd->verify_path[strlen(tsd->verify_path)-1] = 0;

    tsd->config = YAML::LoadFile(tsd->yaml_path);
}

// will create in other process and thread
void init_config_server(const char * recv_path)
{
    pthread_once(&tsd_once_key, tsd_once);
    tsd_t * tsd = new tsd_t;
    pthread_setspecific(tsd_key, tsd);

    const char *    home_path = getenv("HOME");

    memset(tsd->yaml_path, 0, sizeof(tsd->yaml_path));
    snprintf(tsd->yaml_path, sizeof(tsd->yaml_path), "%s/%s", home_path, ".config/lurk/lurk_client_config.yaml");
    make_dir(tsd->yaml_path);

    memset(tsd->verify_path, 0, sizeof(tsd->verify_path));
    snprintf(tsd->verify_path, sizeof(tsd->verify_path), "%s/%s", home_path, ".cache/lurk/server");
    tsd->verify_path[strlen(tsd->verify_path)] = '/';
    make_dir(tsd->verify_path);
    tsd->verify_path[strlen(tsd->verify_path)-1] = 0;

    char server_recv_path[4096];
    strncpy(server_recv_path, recv_path, sizeof(server_recv_path));
    if (server_recv_path[strlen(server_recv_path)] != '/') {
        server_recv_path[strlen(server_recv_path)] = '/';
        make_dir(server_recv_path);
        server_recv_path[strlen(server_recv_path)-1] = 0;
        MSG_DBG_F("%s", recv_path);
    } else {
        make_dir(recv_path);
    }
    char        server_ip[NI_MAXHOST];
    uint16_t    server_port;
    uint16_t    server_deliver_file_port1;
    uint16_t    server_deliver_file_port2;
    uint16_t    server_deliver_file_port3;
    uint16_t    server_deliver_file_port4;

    // check_common_error((server_port = ), const char *error_msg)
    if ( (server_port = get_available_port()) == 0)
        throw std::runtime_error("get available port failed");

    if ( (server_deliver_file_port1 = get_available_port()) == 0)
        throw std::runtime_error("get available port failed");

    if ( (server_deliver_file_port2 = get_available_port()) == 0)
        throw std::runtime_error("get available port failed");

    if ( (server_deliver_file_port3 = get_available_port()) == 0)
        throw std::runtime_error("get available port failed");


    if ( (server_deliver_file_port4 = get_available_port()) == 0)
        throw std::runtime_error("get available port failed");


    memset(server_ip, 0, sizeof(server_ip));
    if ( get_available_ip(server_ip) == 0)
        throw std::runtime_error("get available ip failed");

    // config = YAML::LoadFile(yaml_tmp_path);
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "recv_path";
    out << YAML::Value << recv_path;

    out << YAML::Key << "server_ip";
    out << YAML::Value << server_ip;
    out << YAML::Key << "server_port";
    out << YAML::Value << server_port;
    out << YAML::Key << "server_deliver_file_port1";
    out << YAML::Value << server_deliver_file_port1;
    out << YAML::Key << "server_deliver_file_port2";
    out << YAML::Value << server_deliver_file_port2;
    out << YAML::Key << "server_deliver_file_port3";
    out << YAML::Value << server_deliver_file_port3;
    out << YAML::Key << "server_deliver_file_port4";
    out << YAML::Value << server_deliver_file_port4;

    out << YAML::EndMap;

    std::ofstream fout(tsd->yaml_path);
    fout << out.c_str();
    fout.close();

    printf("Init server environment success.\n");
    printf("You should paste this content to your client config file (~/.config/lurk/lurk_server_config.yaml): \n");
    printf("server_ip : %s\n", server_ip);
    printf("server_port: %hu\n", server_port);
    printf("server_deliver_file_port1: %hu\n", server_deliver_file_port1);
    printf("server_deliver_file_port2: %hu\n", server_deliver_file_port2);
    printf("server_deliver_file_port3: %hu\n", server_deliver_file_port3);
    printf("server_deliver_file_port4: %hu\n", server_deliver_file_port4);

    tsd->config = YAML::LoadFile(tsd->yaml_path);
}


// for server only
void get_config_recv_path_server(char recv_path[], size_t recv_path_len)
{
    tsd_t * tsd = (tsd_t *)pthread_getspecific(tsd_key);
    std::string path = tsd->config["recv_path"].as<std::string>();
    strncpy(recv_path, path.c_str(), path.size());
}


uint16_t get_config_deliver_file_port(int number)
{
    tsd_t * tsd = (tsd_t *)pthread_getspecific(tsd_key);
    char deliver_file_port_str[100];
    memset(deliver_file_port_str, 0, sizeof(deliver_file_port_str));
    snprintf(deliver_file_port_str, sizeof(deliver_file_port_str), "%s%d", "server_deliver_file_port", number);

    uint16_t port = tsd->config[deliver_file_port_str].as<uint16_t>();
    return port;
}

void get_config_verifypath(char verify_path[], size_t verify_path_len)
{
    tsd_t * tsd = (tsd_t *)pthread_getspecific(tsd_key);

    strncpy(verify_path, tsd->verify_path, verify_path_len);
}






void get_config_server_ip(char server_ip_str[], size_t server_ip_str_len)
{
    tsd_t * tsd = (tsd_t *)pthread_getspecific(tsd_key);
    std::string path = tsd->config["server_ip"].as<std::string>();
    strncpy(server_ip_str, path.c_str(), server_ip_str_len);
}


uint16_t get_config_server_port()
{
    tsd_t * tsd = (tsd_t *)pthread_getspecific(tsd_key);
    uint16_t port = tsd->config["server_port"].as<uint16_t>();
    return port;
}
