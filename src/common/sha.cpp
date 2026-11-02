#include "sha.h"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>
#include <functional>
#include <iostream>
#include <cstring>

extern "C" {
    #include <arpa/inet.h>
}

using namespace std;

/*
 * @brief   循环右移一个32位数
 */
static uint32_t
rightrotate32(const uint32_t & num, const uint32_t offset);

/*
 * @brief   将数据进行预处理，使其为512的整数倍
 * @note    数据需要是复制过来的，不应该将原始数据传入此
 */
static int
preprocess(string & data);

/*
 * @brief   将512bit的区块chunk转为64个特定的32位word，w数组带回结果
 */
static int
chunk_make(array<uint32_t, 64> & w, string & data, int chunk_begin);

/*
 * @brief   进行hash迭代计算h数组
 */
static int
hash_iteration(array<uint32_t, 8> & h, const array<uint32_t, 64> & w);



int
sha256(char hash[], const char data[], const int data_len) {
    string                  d(data, data_len);
    array<uint32_t, 64>     w;
    // 哈希初值
    array<uint32_t, 8>      hash_array = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    stringstream            hash_stream;

    if ( preprocess(d) == -1)
        return (-1);
    // 512位(64byte)一组处理所有的区块chunk
    for (int chunk_begin = 0; chunk_begin < d.size(); chunk_begin+=64) {
        if ( chunk_make(w, d, chunk_begin) == -1)
            return (-1);
        if ( hash_iteration(hash_array, w) == -1)
            return (-1);
    }
    // 经过哈希迭代后，hash_arry数组存放最终的hash值
    // 输出256位哈希值到hash中
    hash_stream << std::hex << std::setfill('0');
    for (int idx = 0; idx < hash_array.size(); idx++) {
        hash_stream << std::setw(8) << hash_array[idx];
    }

    // 将64字节拷贝进去
    memcpy(hash, hash_stream.str().c_str(), 64);
    return (0);
}





static uint32_t
rightrotate32(const uint32_t & num, const uint32_t offset) {
    int rtn = (num >> offset) | (num << (32-offset));
    return rtn;
}

/*
 * @brief   将数据进行预处理，使其为512的整数倍
 * @note    数据需要是复制过来的，不应该将原始数据传入此
 */
static int
preprocess(string & data) {
    if (data.size() == 0)
        return (-1);
    // 不规则的最后一部分数据位数
    int         extra_bits = (data.size() * 8) % 512;
    // 填充的字节数，中间部分小于448时填充到448（预留64bit作为数据长度信息），大于448不满512时填充到512+448处
    int         append_bits = (extra_bits < 448 ? (448-extra_bits) : (512+448-extra_bits)) + 64;
    int         append_begin = data.size();
    int         size = data.size();
    // 用于存入最后64位
#if defined (__APPLE__) || defined (__FreeBSD__)
    uint64_t    size_to_append = htonll(data.size()*8);
#elif defined (__linux__)
    uint64_t    size_to_append = htonbe64(data.size()*8);
#endif

    data.resize(size+(append_bits/8), 0);
    data[append_begin] = 0x80; //  添加一位1，后面全跟着的是0
    memcpy(&data[data.size()-8], &size_to_append, 8);

    return (0);
}

/*
 * @brief   将512bit的区块chunk转为64个特定的32位word，w数组带回结果
 */
static int
chunk_make(array<uint32_t, 64> & w, string & data, int chunk_begin) {
    // 前16个w[i]就是512bit chunk的分块
    for (int i = 0; i < 16; ++i) {
            size_t idx = chunk_begin + i * 4;
            // 保证以大端序形式排列，"<<"永远保证将低地址的data[idx]对应的1个字节(8bit)移动到最高位，大端小端机器均通用
            uint32_t val =
                (static_cast<uint8_t>(data[idx])     << 24) |
                (static_cast<uint8_t>(data[idx + 1]) << 16) |
                (static_cast<uint8_t>(data[idx + 2]) <<  8) |
                (static_cast<uint8_t>(data[idx + 3]));
            w[i] = val;
        }
    // 后48个w[i]根据特定迭代公式进行构建
    for (int idx = 16; idx < 64; idx++) {
        uint32_t sigma_0 = rightrotate32(w[idx-15], 7) ^ rightrotate32(w[idx-15], 18) ^ (w[idx-15] >> 3);
        uint32_t sigma_1 = rightrotate32(w[idx-2], 17) ^ rightrotate32(w[idx-2], 19) ^ (w[idx-2] >> 10);
        w[idx] = w[idx-16] + sigma_0 + w[idx-7] + sigma_1;
    }
    return (0);
}

/*
 * @brief   进行hash迭代计算h数组
 */
static int
hash_iteration(array<uint32_t, 8> & h, const array<uint32_t, 64> & w) {
    if (w.size() < 64)
        return (-1);


    // 轮常量
     static const array<const uint32_t, 64> round_constants = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };


    // 用到的迭代公式
    function<uint32_t(uint32_t, uint32_t, uint32_t)>    Ch, M;
    function<uint32_t(uint32_t)>                        sum0, sum1;
    Ch = [](uint32_t x, uint32_t y, uint32_t z)->uint32_t{
        return (x & y) ^ (~x & z);
    };
    M = [](uint32_t x, uint32_t y, uint32_t z)->uint32_t{
        return (x & y) ^ (x & z) ^ (y & z);
    };
    sum0 = [](uint32_t x)->uint32_t {
        return rightrotate32(x, 2) ^ rightrotate32(x, 13) ^ rightrotate32(x, 22);
    };
    sum1 = [](uint32_t x)->uint32_t {
        return rightrotate32(x, 6) ^ rightrotate32(x, 11) ^ rightrotate32(x, 25);
    };

    array<uint32_t, 8> h_next = h;

    // 迭代64次形成最后的hash
    for (int idx = 0; idx < 64; idx++) {
        // 此处使用轮常量计算中间结果
        uint32_t inter_result0 = h_next[7] + sum1(h_next[4]) + Ch(h_next[4], h_next[5], h_next[6]) + round_constants[idx] + w[idx];
        uint32_t inter_result1 = sum0(h_next[0]) + M(h_next[0], h_next[1], h_next[2]);

        // for (int hi = h_next.size()-1; hi >= 0; hi--) {
        //     if (hi == 0) {
        //         h_next[hi] = inter_result0 + inter_result1;
        //     } else if (hi == 4) {
        //         h_next[hi] = h_next[3] + inter_result0;
        //     } else {
        //         h_next[hi] = h_next[hi-1];
        //     }
        // }
        h_next[7] = h_next[6];
        h_next[6] = h_next[5];
        h_next[5] = h_next[4];
        h_next[4] = h_next[3] + inter_result0;
        h_next[3] = h_next[2];
        h_next[2] = h_next[1];
        h_next[1] = h_next[0];
        h_next[0] = inter_result0 + inter_result1;
    }
    for (int hi = 0; hi < h.size(); hi++) {
        h[hi] += h_next[hi];
    }

    // // 生成最后的hash
    // hash_stream << std::hex;
    // for (int idx = 0; idx < 8; idx++) {
    //     hash_stream <<  htonl(h[idx]);
    // }

    return (0);
}
