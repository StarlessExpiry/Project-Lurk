#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// create path (all dirs this path involved),
// but whether the end of character is '\' or not determines if the last 'dir' should be created;
// e.g. create path "/Users/starless/.config" will try create dir "/Users/" and "/Users/starless/",
// e.g. create path "/Users/starless/.config/" will try create dir "Users/" and "/Users/starless/" and "/Users/starless/.config/"
bool
make_dir(const char * path);

#ifdef __cplusplus
}
#endif
