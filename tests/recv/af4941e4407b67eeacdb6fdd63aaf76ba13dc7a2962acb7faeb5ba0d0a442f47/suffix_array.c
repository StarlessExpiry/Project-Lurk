#include "suffix_array.h"
#include <string.h>
#include <unistd.h>
int
init_suffix_array(struct suffix_array * suf_arr)
{
    return 0;
}

void getsa(struct suffix_array * suf_arr)
{

    memset(suf_arr->sa, 0, sizeof(int) * suf_arr->length);
    memset(suf_arr->rank, 0, sizeof(int) * suf_arr->length);
    memset(suf_arr->height, 0, sizeof(int) * suf_arr->length);

    int tmp[suf_arr->length];

}
