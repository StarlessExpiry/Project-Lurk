#include "tools/get_config.h"
#include <cstdio>
#include <cstdlib>
int main()
{
    if ( init_config_client() == false) {
        printf("😰 lurk init failed\n");
        return EXIT_FAILURE;
    }


    printf("🫡 lurk init success\n");
    return EXIT_SUCCESS;
}
