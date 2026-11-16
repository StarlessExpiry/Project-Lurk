#include "discret.hpp"
#include <algorithm>
#include <unordered_map>
#include <vector>

int
discretize(int *arr, int length)
{
    if (length <= 0) {
        return -1;
    }
    std::vector<int> tmp(arr, arr + length);
    std::sort(tmp.begin(), tmp.end());
    std::unordered_map<int, int> hmap;
    int idx = 0;
    for (int x : tmp) {
        if (hmap.find(x) == hmap.end()) {
            hmap[x] = idx++;
        }
    }
    for (int i = 0; i < length; ++i) {
        arr[i] = hmap[arr[i]];
    }
    return 0;
}
