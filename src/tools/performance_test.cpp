#include "performance_test.hpp"
#include <time.h>



void Performance_test::begin() {
    done_flag = 0;
    clock_gettime(CLOCK_MONOTONIC, &time_start);
    cpu_start = clock();
}

void Performance_test::end() {
    done_flag = 0;
    clock_gettime(CLOCK_MONOTONIC, &time_end);
    cpu_end = clock();
}

pt_t Performance_test::get_times() {
    if (done_flag == 1) {
        return pt;
    }
    pt.cpu_duration = (double)(cpu_end - cpu_start) / CLOCKS_PER_SEC;
    pt.time_duration = (time_end.tv_sec - time_start.tv_sec) + (time_end.tv_nsec - time_start.tv_nsec) / 1e9;
    done_flag = 1;
    return pt;
}


double calculate_throughout(uint64_t file_size, double time_duration)
{

    if (time_duration == (double)0.0) {
        return ((double)file_size / (1024.0 * 1024.0)) / time_duration;
    } else {
        return 0.0;
    }
}
