#include "../sys/msg_print.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// 接收空间大小和发送空间大小
#define MAXDATA 1024

// void
// str_echo_dgram_cli(FILE *io, int sockfd, struct sockaddr *pservaddr,
//                    socklen_t servlen) {
//     char sendline[MAXDATA], recvline[MAXDATA];
//     int  read_cnt;
//     while (fgets(sendline, MAXDATA, io) != NULL) {
//         Sendto(sockfd, sendline, strlen(sendline), NULL, pservaddr, servlen);
//         read_cnt = Recvfrom(sockfd, recvline, MAXDATA, NULL, NULL, NULL);

//         recvline[read_cnt] = '\0'; // recvfrom同样不会给字符串字符串末尾添加\0
//         fputs(recvline, stdout);
//         fflush(stdout);
//         bzero(recvline, MAXDATA);
//     }
// }

// static void	sig_alrm(int);
//
// void
// str_echo_dgram_cli_timeo(FILE * fp_input, FILE * fp_output, int sockfd, const
// struct sockaddr *pservaddr, socklen_t servlen, int sec)
// {
// 	int		n;
// 	char	sendline[MAXDATA], recvline[MAXDATA + 1];
//
// 	Signal(SIGALRM, sig_alrm);
//
// 	while (fgets(sendline, MAXDATA, fp_input) != NULL) {
//
// 		Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
//
// 		alarm(sec);
// 		if ( (n = recvfrom(sockfd, recvline, MAXDATA, NULL, NULL, NULL)) < 0) {
// 			if (errno == EINTR) {
//                 errno = ETIMEDOUT;
//                 msg_err("servs/str_echo/str_echo_dgram_cli_timeo: timeout");
//             }
// 			else
// 				msg_err("servs/str_echo/str_echo_dgram_cli_timeo: recvfrom
// error"); 		} else { 			alarm(0); 			recvline[n] = 0;	/* null terminate */
// 			fputs(recvline, fp_output); // 输出到终端上
// 		}
// 	}
// }
//
// static void
// sig_alrm(int signo)
// {
// 	return;			/* just interrupt the recvfrom() */
// }

// void
// str_echo_dgram_cli_timeo(FILE *fp_input, FILE *fp_output, int sockfd,
//                          const struct sockaddr *pservaddr, socklen_t servlen,
//                          int sec) {
//     int  n;
//     char sendline[MAXDATA], recvline[MAXDATA + 1];

//     while (fgets(sendline, MAXDATA, fp_input) != NULL) {

//         Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
//         if (wait_fd_readable_timeo(sockfd, sec)) {
//             msg_err("servs/str_echo/str_echo_dgram_cli_timeo: timeout");
//             exit(EXIT_FAILURE);
//         } else {
//             n           = Recvfrom(sockfd, recvline, MAXDATA, NULL, NULL, NULL);
//             recvline[n] = '\0';
//             fputs(recvline, fp_output);
//         }
//     }
// }

// void
// str_echo_dgram_serv(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen) {
//     char buf[1024];
//     while (1) {
//         int read_cnt = Recvfrom(sockfd, buf, 1024, NULL, pcliaddr, &clilen);
//         Sendto(sockfd, buf, read_cnt, NULL, pcliaddr, clilen);

//         bzero(buf, 1024);
//     }
// }
