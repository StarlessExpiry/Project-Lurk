#pragma once
// 类型:    用来构建后缀数组的结构体
// 参数:    char * str      输入字符串
struct suffix_array {
    char *str;
    int   length;
    int  *sa;
    int  *rank;
    int  *height;
};

int
init_suffix_array(struct suffix_array * suf_arr);
