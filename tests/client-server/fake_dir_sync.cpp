#include "core/dir_sync.hpp"

Sync_dir_sender::Sync_dir_sender(int sockfd, uint16_t deliver_file_port, sock_tag_t socktag, const char * dirpath_from_database, const char * verify_rootpath, pid_t leader_pid) {
    return;
}

bool Sync_dir_sender::sync_start() {
    return false;
}

Sync_dir_sender::~Sync_dir_sender() {
    return;
}

bool Sync_dir_sender::send_info() {
    return true;
}


bool Sync_dir_sender::get_info_ack() {
    return true;
}


bool Sync_dir_sender::send_unit() {
    return true;
}


bool Sync_dir_sender::send_unit_entrys() {
    return true;
}


bool Sync_dir_sender::recv_unit_need() {
    return true;
}


bool Sync_dir_sender::send_unit_need() {
    return true;
}



Sync_dir_receiver::Sync_dir_receiver(int sockfd, sock_tag_t socktag, pid_t leader_pid) {
    return;
}

bool Sync_dir_receiver::sync_start() {
    return false;
}

Sync_dir_receiver::~Sync_dir_receiver() {
    return;
}

bool Sync_dir_receiver::recv_info() {
    return true;
}

bool Sync_dir_receiver::send_info_ack() {
    return true;
}

bool Sync_dir_receiver::recv_unit() {
    return true;
}

bool Sync_dir_receiver::recv_unit_and_compare() {
    return true;
}

bool Sync_dir_receiver::send_unit_need() {
    return true;
}

bool Sync_dir_receiver::recv_unit_need() {
    return true;
}
