#include <ctime>
#include <cstdint>

typedef struct {
    double cpu_duration;
    double time_duration;
    double throughput;
} pt_t;

class Performance_test {
public:
    Performance_test() = default;
    void    begin();
    void    end();
    pt_t    get_times();
private:
    int done_flag = 0;
    // for cpu times
    clock_t cpu_start = 0;
    clock_t cpu_end = 0;
    // for actual times (seconds)
    struct timespec time_start = {};
    struct timespec time_end = {};
    pt_t pt = {};
};

double calculate_throughout(uint64_t file_size, double time_duration);