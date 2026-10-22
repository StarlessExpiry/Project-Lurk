#include "deliver/deliver.h"
#include "deliver/file_deliver.h"

// Stub
bool
send_single_file(int sockfd, sock_tag_t socktag, const char * filepath, int basepath_len, char basepath_hash[65], const char * verify_dir) { return true; }

bool
recv_single_file(int sockfd, sock_tag_t socktag, const char * path_to_recv, const char * verify_dir) { return true; }
