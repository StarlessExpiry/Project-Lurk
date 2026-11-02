#include "config/get_config.hpp"
#include <cstdio>
#include <cstdlib>
#include <exception>
int main()
{
try {
    init_config_client();
    printf("🫡 lurk init success\n");
    return EXIT_SUCCESS;
} catch (YAML::Exception & e) {
    
}
    


}
