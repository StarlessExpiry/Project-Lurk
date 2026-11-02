#pragma once
#include <cstdio>
#include <unistd.h>


struct fd_deleter {
    void operator()(int * fd) const {
        if (fd) {
            if (*fd >= 0) {
                close(*fd);
            }
            delete fd;
        }
    }
};


struct stream_deleter {
    void operator()(FILE * stream) const {
        if (stream) {
            fclose(stream);
        }
    }
};
