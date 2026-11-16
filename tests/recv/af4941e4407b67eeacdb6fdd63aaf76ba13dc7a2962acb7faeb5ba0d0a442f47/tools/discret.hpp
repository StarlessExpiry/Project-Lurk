#pragma once

extern "C" {
// 函数:    离散化一个整数数组
// 参数:    int * arr   整数数组
//          int length  数组长度
// 返回值:  int         若length小于等于0则返回-1，否则返回0
int
discretize(int *arr, int length);

}
