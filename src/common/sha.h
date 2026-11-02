#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/*
 * @brief               Input an array of characters of any length and output 256bit hash value（char[])
 *
 * @param[in]           data array
 * @param[in, out]      hash array which has already pre-allocate 256bit memory(64bytes), not end with '\0'
 */
int
sha256(char hash[], const char data[], const int data_len);

#ifdef __cplusplus
}
#endif
