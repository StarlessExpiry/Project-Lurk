#include "get_config.h"
#include "get_available_port.h"
#include "makedir.h"
#include "sys/msg_print.h"
#include "yaml-cpp/yaml.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sys/stat.h>


static char        yaml_path[4096];
static YAML::Node  config;

// should do nothing , just get from server config file
bool init_config_client()
{
    const char *    home_path = getenv("HOME");
    char            yaml_tmp_path[4096];
    memset(yaml_tmp_path, 0, sizeof(yaml_tmp_path));
    snprintf(yaml_tmp_path, sizeof(yaml_tmp_path), "%s/%s", home_path, ".config/lurk/lurk_client_config.yaml");
    make_dir(yaml_tmp_path);

    char verify_dir[4096];
    // char            yaml_tmp_path[4096];
    // memset(yaml_tmp_path, 0, sizeof(yaml_tmp_path));
    snprintf(verify_dir, sizeof(verify_dir), "%s/%s", home_path, ".cache/lurk/client");
    verify_dir[strlen(verify_dir)] = '/';
    make_dir(verify_dir);
    verify_dir[strlen(verify_dir)-1] = 0;
//
//     char            verify_path[4096];
//     memset(verify_path, 0, sizeof(verify_path));
//     snprintf(verify_path, sizeof(verify_path), "%s/%s", home_path, ".cache/lurk/client");
//     verify_path[strlen(verify_path)] = '/';
//     make_dir(verify_path);
//     verify_path[strlen(verify_path)-1] = 0;
//     MSG_DBG_F("verify_path is %s", verify_path);
//
//     try {
//         // config = YAML::LoadFile(yaml_tmp_path);
//         YAML::Emitter out;
//         out << YAML::BeginMap;
//         out << YAML::Key << "verify_path";
//         out << YAML::Value << verify_path;
//
//         out << YAML::Key << "port";
//         out << YAML::Value << "20210";
//         out << YAML::EndMap;
//
//         std::ofstream fout(yaml_tmp_path);
//         fout << out.c_str();
//         fout.close();
//         MSG_DBG_F("file '%s' < \n%s", yaml_tmp_path, out.c_str());
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("INIT config file failed", e.what());
//         return false;
//     }
    return true;
}

bool init_config_server(char * recv_path)
{
    const char *    home_path = getenv("HOME");
    char            yaml_tmp_path[4096];
    memset(yaml_tmp_path, 0, sizeof(yaml_tmp_path));
    snprintf(yaml_tmp_path, sizeof(yaml_tmp_path), "%s/%s", home_path, ".config/lurk/lurk_server_config.yaml");
    make_dir(yaml_tmp_path);

    // char            verify_path[4096];
    // memset(verify_path, 0, sizeof(verify_path));
    // snprintf(verify_path, sizeof(verify_path), "%s/%s", home_path, ".cache/lurk/server");
    if (recv_path[strlen(recv_path)] != '/') {
        recv_path[strlen(recv_path)] = '/';
        make_dir(recv_path);
        recv_path[strlen(recv_path)-1] = 0;
        MSG_DBG_F("%s", recv_path);
    } else {
        make_dir(recv_path);
    }

    char verify_dir[4096];
    // char            yaml_tmp_path[4096];
    // memset(yaml_tmp_path, 0, sizeof(yaml_tmp_path));
    snprintf(verify_dir, sizeof(verify_dir), "%s/%s", home_path, ".cache/lurk/server");
    verify_dir[strlen(verify_dir)] = '/';
    make_dir(verify_dir);
    verify_dir[strlen(verify_dir)-1] = 0;

    try {
        // config = YAML::LoadFile(yaml_tmp_path);
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "recv_path";
        out << YAML::Value << recv_path;

        char        server_ip[NI_MAXHOST];
        uint16_t    server_port;
        char        server_port_str[10];
        uint16_t    server_deliver_file_port1;
        uint16_t    server_deliver_file_port2;
        uint16_t    server_deliver_file_port3;
        uint16_t    server_deliver_file_port4;

        if ( (server_port = get_available_port()) == false) {
            MSG_DBG_F("get available port failed");
            return false;
        }

        if ( (server_deliver_file_port1 = get_available_port()) == false) {
            MSG_DBG_F("get available port failed");
            return false;
        }

        if ( (server_deliver_file_port2 = get_available_port()) == false) {
            MSG_DBG_F("get available port failed");
            return false;
        }

        if ( (server_deliver_file_port3 = get_available_port()) == false) {
            MSG_DBG_F("get available port failed");
            return false;
        }


        if ( (server_deliver_file_port4 = get_available_port()) == false) {
            MSG_DBG_F("get available port failed");
            return false;
        }

        snprintf(server_port_str, sizeof(server_port_str), "%hu", server_port);

        memset(server_ip, 0, sizeof(server_ip));
        if ( get_available_ip(server_ip) == false) {
            MSG_DBG_F("get available ip failed");
            return false;
        }

        out << YAML::Key << "server_ip";
        out << YAML::Value << server_ip;
        out << YAML::Key << "server_port";
        out << YAML::Value << server_port_str;
        out << YAML::Key << "server_deliver_file_port1";
        out << YAML::Value << server_deliver_file_port1;
        out << YAML::Key << "server_deliver_file_port2";
        out << YAML::Value << server_deliver_file_port2;
        out << YAML::Key << "server_deliver_file_port3";
        out << YAML::Value << server_deliver_file_port3;
        out << YAML::Key << "server_deliver_file_port4";
        out << YAML::Value << server_deliver_file_port4;

        out << YAML::EndMap;

        std::ofstream fout(yaml_tmp_path);
        fout << out.c_str();
        fout.close();

        printf("Init server environment success.\n");
        printf("You should paste this content to your client config file (~/.config/lurk/lurk_server_config.yaml): \n");
        printf("server_ip : %s\n", server_ip);
        printf("server_port: %s\n", server_port_str);
        printf("server_deliver_file_port1: %hu\n", server_deliver_file_port1);
        printf("server_deliver_file_port2: %hu\n", server_deliver_file_port2);
        printf("server_deliver_file_port3: %hu\n", server_deliver_file_port3);
        printf("server_deliver_file_port4: %hu\n", server_deliver_file_port4);
    } catch (const YAML::Exception& e) {
        MSG_DBG_F("INIT config file failed: %s", e.what());
        return false;
    }
    return true;
}

static bool reread_config_client()
{
    const char *  home_path = getenv("HOME");
    memset(yaml_path, 0, sizeof(yaml_path));
    snprintf(yaml_path, sizeof(yaml_path), "%s/%s", home_path, ".config/lurk/lurk_client_config.yaml");

    try {
    config = YAML::LoadFile(yaml_path);
    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed: %s", e.what());
        return false;
    }
    return true;
}

static bool reread_config_server()
{
    const char *  home_path = getenv("HOME");
    memset(yaml_path, 0, sizeof(yaml_path));
    snprintf(yaml_path, sizeof(yaml_path), "%s/%s", home_path, ".config/lurk/lurk_server_config.yaml");

    try {
    config = YAML::LoadFile(yaml_path);
    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return false;
    }
    return true;
}

uint16_t get_config_deliver_file_port(int number)
{
    try {

    if ( reread_config_client() == false)
        return 0;
    char deliver_file_port_str[100];
    memset(deliver_file_port_str, 0, sizeof(deliver_file_port_str));
    snprintf(deliver_file_port_str, sizeof(deliver_file_port_str), "%s%d", "server_deliver_file_port", number);
    MSG_DBG_F("server_deliver_file_port = %s", deliver_file_port_str);
    uint16_t port = config[deliver_file_port_str].as<uint16_t>();
    return port;

    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return 0;
    }

}
// uint16_t get_config_port_client()
// {
//     try {
//
//     if ( reread_config_client() == false)
//         return 0;
//     uint16_t port = config["port"].as<uint16_t>();
//     return port;
//
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("Read config file failed", e.what());
//         return 0;
//     }
// }

// bool set_config_port_client(uint16_t port)
// {
//     try {
//         if ( reread_config_client() == false)
//             return false;
//         config["port"] = port;
//
//         std::ofstream fout(yaml_path);
//         fout << config;
//         fout.close();
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("Set config file failed", e.what());
//         return false;
//     }
//     return true;
// }

bool get_config_verifypath_client(char * verify_path)
{
//     try {
//
//     if ( reread_config_client() == false)
//         return false;
//     std::string path = config["verify_path"].as<std::string>();
//     strncpy(verify_path, path.c_str(), path.size());
//     return true;
//
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("Read config file failed", e.what());
//         return 0;
//     }
    // cancel getting from config file
    const char *    home_path = getenv("HOME");
    // char            yaml_tmp_path[4096];
    // memset(yaml_tmp_path, 0, sizeof(yaml_tmp_path));
    snprintf(verify_path, 100, "%s/%s", home_path, ".cache/lurk/client");

    return true;
}



bool get_config_verifypath_server(char * verify_path)
{

    // cancel getting from config file
    const char *    home_path = getenv("HOME");
    // char            yaml_tmp_path[4096];
    // memset(yaml_tmp_path, 0, sizeof(yaml_tmp_path));
    snprintf(verify_path, 100, "%s/%s", home_path, ".cache/lurk/server");

    return true;
}

bool get_config_recv_path_server(char * recv_path)
{
    try {
    if ( reread_config_server() == false)
        return false;
    std::string path = config["recv_path"].as<std::string>();
    strncpy(recv_path, path.c_str(), path.size());
    return true;

    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return 0;
    }
}




// bool set_config_verifypath_client(char * verify_path)
// {
//     try {
//         if ( reread_config_client() == false)
//             return false;
//         config["verify_path"] = std::string(verify_path);
//
//         std::ofstream fout(yaml_path);
//         fout << config;
//         fout.close();
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("Set config file failed", e.what());
//         return false;
//     }
//     return true;
// }


bool get_config_server_ip_client(char * server_ip_str)
{
    try {
        if ( reread_config_client() == false)
            return false;
        std::string path = config["server_ip"].as<std::string>();
        strncpy(server_ip_str, path.c_str(), path.size());
        return true;
    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return 0;
    }
}

bool get_config_server_ip_server(char * server_ip_str)
{
    try {
        if ( reread_config_server() == false)
            return false;
        std::string path = config["server_ip"].as<std::string>();
        strncpy(server_ip_str, path.c_str(), path.size());
        return true;
    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return 0;
    }
}

// bool set_config_server_ip_client(const char * server_ip_str)
// {
//     try {
//         if ( reread_config_client() == false)
//             return false;
//         config["server_ip"] = std::string(server_ip_str);
//
//         std::ofstream fout(yaml_path);
//         fout << config;
//         fout.close();
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("Set config file failed", e.what());
//         return false;
//     }
//     return true;
// }

uint16_t get_config_server_port_client()
{
    try {

    if ( reread_config_client() == false)
        return 0;
    uint16_t port = config["server_port"].as<uint16_t>();
    return port;

    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return 0;
    }
}


uint16_t get_config_server_port_server()
{
    try {

    if ( reread_config_server() == false)
        return 0;
    uint16_t port = config["server_port"].as<uint16_t>();
    return port;

    } catch (const YAML::Exception& e) {
        MSG_DBG_F("Read config file failed", e.what());
        return 0;
    }
}

// bool set_config_server_port_client(uint16_t port)
// {
//     try {
//         if ( reread_config_client() == false)
//             return false;
//         config["server_port"] = port;
//
//         std::ofstream fout(yaml_path);
//         fout << config;
//         fout.close();
//     } catch (const YAML::Exception& e) {
//         MSG_DBG_F("Set config file failed", e.what());
//         return false;
//     }
//     return true;
// }
