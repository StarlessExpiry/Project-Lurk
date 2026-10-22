#include "tools/get_available_port.h"
#include "sys/msg_print.h"
#include "tools/makedir.h"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <spawn.h>
#include <string.h>

extern "C" {
    #include "tools/get_config.h"
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/tcp.h>
}

// for the communication of lurk_server_single_file_deliver and lurk_server_dir_sync
// two processes use same fileno to talk
#define SYNC_FILENO 20

static bool precreate_file_process();

int main(int argc, char * argv[])
{
    if ( precreate_file_process() == false) {
        MSG_DBG_F("create process to recv file failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in  myaddr;
    int                 listenfd;
    uint16_t            server_port;
    char                server_ip_str[20];

    server_port = get_config_server_port_server();
    if ( server_port == 0) {
        MSG_DBG_F("Read server_port in your config file failed");
        return EXIT_FAILURE;
    }

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(server_port);

    memset(server_ip_str, 0, sizeof(server_ip_str));
    if (get_config_server_ip_server(server_ip_str) == false) {
        printf("Get server_ip in your config file failed.\n");
        return EXIT_FAILURE;
    }

    inet_pton(AF_INET, server_ip_str, &myaddr.sin_addr);

    printf("server_ip is %s, server_port is %hu\n", server_ip_str, server_port);

    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        MSG_DBG_F("Socket create error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    if ( bind(listenfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
        MSG_DBG_F("Bind error: %s", strerror(errno));
        return EXIT_FAILURE;
    }
    if ( listen(listenfd, 20) == -1) {
        MSG_DBG_F("Listen error: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    while (1) {
        int connfd;
        struct sockaddr_in  peeraddr;
        socklen_t           peeraddr_len = sizeof(peeraddr);
        if ( (connfd = accept(listenfd, (struct sockaddr *)&peeraddr, &peeraddr_len)) == -1) {
            MSG_DBG_F("Accept failed: %s", strerror(errno));
            return EXIT_FAILURE;
        }

        uint8_t     recv_message;
        if ( recv(connfd, &recv_message, sizeof(recv_message), 0) == -1) {
            MSG_DBG_F("recv failed: %s", strerror(errno));
            return EXIT_FAILURE;
        }
        if (recv_message == 0x01) {
            uint16_t port;
            if ( (port = get_available_port()) == 0) {
                MSG_DBG_F("Port cannot be allocate: %s", strerror(errno));
                MSG_DBG("Close Connection");
                close(connfd);
                continue;
            }
            MSG_DBG_F("Next Connection Port is %d", port);


            char port_str[10];
            memset(port_str, 0, sizeof(port_str));
            snprintf(port_str, sizeof(port_str), "%hu", port);
            MSG_DBG_F("Connect port is %s", port_str);


            // here to creat a sync process
            pid_t   pid = fork();
            if ( pid == -1) {
                MSG_DBG_F("Fork error: %s", strerror(errno));
            } else if (pid == 0) {// child
                execl("./lurk_server_dir_sync", "lurk_server_dir_sync", port_str, "0", NULL);
                MSG_DBG_F("execl error: %s", strerror(errno));

            } else {
                sleep(2);
                port = htons(port);
                if ( send(connfd, &port, sizeof(port), 0) == -1) {
                    MSG_DBG_F("Send error: %s", strerror(errno));
                    MSG_DBG("Close Connection");
                    close(connfd);
                    continue;
                }
                port = ntohs(port);

                // finish connection, but waiting for client close
                char tmp[10];
                while ( read(connfd, tmp, 10) > 0) {
                    // do nothing
                } // until read() == 0 or -1

                close(connfd);
            }
        }
    }

}
extern char ** environ;

// the recv dir should set in here, give it to file deliver process
static bool precreate_file_process()
{
    MSG_DBG_F("leader_pid = %d", getpid());
    for (int i = 1; i <= 4; i++) {
        uint16_t    deliver_file_port = get_config_deliver_file_port(i);
        char        deliver_file_port_str[10];
        char        verify_dir[4096];
        char        recv_dir[4096];


        if (deliver_file_port == 0) {
            MSG_DBG_F("deliver_file_port%d get failed", i);
            return false;
        }
        memset(deliver_file_port_str, 0, sizeof(deliver_file_port_str));
        snprintf(deliver_file_port_str, sizeof(deliver_file_port_str), "%hu", deliver_file_port);

        memset(verify_dir, 0, sizeof(verify_dir));
        if ( get_config_verifypath_server(verify_dir) == false) {
            MSG_DBG_F("get verify_path in config file failed");
            return false;
        }

        // verify_dir[strlen(verify_dir)] = '/';
        // make_dir(verify_dir);
        // verify_dir[strlen(verify_dir)-1] = 0;
        memset(recv_dir, 0, sizeof(recv_dir));
        if ( get_config_recv_path_server(recv_dir) == false) {
            MSG_DBG_F("get recv_path in config file failed");
            return false;
        }

        // Pe-created process to recv file
        char *  argv_str[6];

        argv_str[0] = (char *)"lurk_server_single_file_deliver";
        argv_str[1] = (char *)"ST_COMMON";
        argv_str[2] = (char *)deliver_file_port_str;
        argv_str[3] = (char *)recv_dir;
        argv_str[4] = (char *)verify_dir;
        argv_str[5] = NULL;
        MSG_DBG_F("recv dir is %s", recv_dir);

        pid_t pid_rtn;
        posix_spawn_file_actions_t      process_action; // Redirect the send_single_file process's stdout to the current process's stdout/stderror
        posix_spawnattr_t               process_attr; // enable the process join a same process group, it is convenient to kill all of them
        posix_spawn_file_actions_init(&process_action);

        posix_spawnattr_init(&process_attr);
        posix_spawnattr_setflags(&process_attr, POSIX_SPAWN_SETPGROUP);
        posix_spawnattr_setpgroup(&process_attr, getpid());

        posix_spawn_file_actions_adddup2(&process_action, STDOUT_FILENO, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&process_action, STDERR_FILENO, STDERR_FILENO);

        if ( posix_spawn(&pid_rtn, "./lurk_server_single_file_deliver", &process_action, &process_attr, argv_str, environ) != 0) {
            MSG_DBG_F("create process errro : %s", strerror(errno));
            return EXIT_FAILURE;
        }
        MSG_DBG_F("lurk_server_single_file_deliver pid = %d", pid_rtn);
        MSG_DBG_F("create process success, pid = %d", pid_rtn);

        posix_spawnattr_destroy(&process_attr);
        posix_spawn_file_actions_destroy(&process_action);
    }
    return true;
}

// uint16_t get_an_available_port() {
//     int                 sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     struct sockaddr_in  myaddr = {
//         .sin_family = AF_INET,
//         .sin_addr.s_addr = htonl(INADDR_ANY),
//         .sin_port = htons(0)
//     };
//     int                 port;
//
//     if ( bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
//         MSG_DBG_F("Bind error: %s", strerror(errno));
//         return 0;
//     }
//
//     socklen_t           addr_len = sizeof(myaddr);
//     getsockname(sockfd, (struct sockaddr *)&myaddr, &addr_len);
//
//     // get an avaliable port
//     port = ntohs(myaddr.sin_port);
//
//     close(sockfd);
//     return port;
// }
