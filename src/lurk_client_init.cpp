#include "config/get_config.hpp"
#include <cstdio>
#include <cstdlib>
#include <deps/yaml-cpp/include/yaml-cpp/yaml.h>
#include <iostream>
int main()
{
try {
    init_config_client();
    printf("🫡 lurk init success\n");
    return EXIT_SUCCESS;
} catch (YAML::Exception & e) {
    std::cerr << "[lurk init] failed to parse config file: " << e.what() << std::endl;
}

}
