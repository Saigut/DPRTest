#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <pthread.h>
#include <stdbool.h>

#include "common.h"

#define C_PORT 0    // random port

typedef struct {
    int c_fd;
    char * s_ip;
    struct sockaddr_in * p_s_addr;
} hb_param;

void * heartbeat_thread(void * p_param);
int deal_msg(int s_fd, trans_msg * msg, struct sockaddr_in * p_c_addr);

struct sockaddr_in peer_addr;

struct sockaddr_in s_addr;

struct sockaddr_in check_s_addr;

struct sockaddr_in temp_addr;

bool sent_do_hole_pack = false;

int main( int argc, char *argv[] ) {

    char * s_ip = NULL;

    if (argc > 1) {
        s_ip = argv[1];
    } else {
        printf("Please specify server ip on command line.\n");
        printf("Example: ./program 100.100.100.100\n");
        return -1;
    }

    int ret;
    int c_fd;

    memset(&check_s_addr, 0, sizeof(check_s_addr));
    if (argc > 2) {
        check_s_addr.sin_family = AF_INET;
        check_s_addr.sin_addr.s_addr = inet_addr(argv[2]);
        check_s_addr.sin_port = htons(CHECK_S_PORT);
    }

    uint16_t c_port = C_PORT;

    if (argc > 3) {
        c_port = atoi(argv[3]);
    }

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;

    do {

        ret = setup_server(&c_fd, c_port, false);
        if (ret < 0) {
            if (EADDRINUSE != errno) {
                printf("Failed to Set Up Server.\n");
                return -1;
            }
            c_port++;
            if (c_port > 65535) {
                printf("No port for me use.");
                return -1;
            }
        } else {
            break;
        }

    } while (1);

    hb_param param;
    param.c_fd = c_fd;
    param.s_ip = s_ip;
    param.p_s_addr = &s_addr;

    pthread_t pid;
    ret = pthread_create(&pid, NULL, heartbeat_thread, &param);
    if (0 != ret) {
        printf("Failed to Create Thread.");
        return -1;
    }

    return start_server(c_fd, deal_msg);
}

void * heartbeat_thread(void * p_param) {

    int c_fd = ((hb_param *)p_param)->c_fd;
    char * s_ip = ((hb_param *)p_param)->s_ip;
    struct sockaddr_in * p_s_addr = ((hb_param *)p_param)->p_s_addr;


    memset(p_s_addr, 0, sizeof(struct sockaddr_in));
    p_s_addr->sin_family = AF_INET;
    p_s_addr->sin_addr.s_addr = inet_addr(s_ip);
    p_s_addr->sin_port = htons(S_PORT);

    ssize_t sndn = 0;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    trans_msg msg_send;
    msg_send.dollar = '$';
    msg_send.cmd = (int8_t)C_S_HEART_BEAT;
    msg_send.text[0] = 'h';
    msg_send.text[1] = 'i';

    size_t msg_size = sizeof(trans_msg);

    while (1) {
        sndn = sendto(c_fd, &msg_send, msg_size, 0, (struct sockaddr *)p_s_addr, addrlen);
        if (sndn < 0) {
            printf("Failed to Send Heart Beat. Errno: %d\n", errno);
        } else if (sndn == msg_size) {
            printf("Heart Beat to %s:%d Sent.\n",
                   inet_ntoa(p_s_addr->sin_addr), ntohs(p_s_addr->sin_port));
        } else {
            printf("Heart Beat Not Send Completely.\n");
        }

        usleep(10000000);
    }
}

int deal_msg(int s_fd, trans_msg * msg, struct sockaddr_in * p_c_addr) {
    if (NULL == msg) {
        printf("%s:%s msg is NULL", __FILE__, __FUNCTION__);
        return -1;
    }

    if (NULL == p_c_addr) {
        printf("%s:%s p_c_addr is NULL", __FILE__, __FUNCTION__);
        return -1;
    }

    printf("Received Massage: %c%c, From: %s:%d\n",
           msg->text[0], msg->text[1],
           inet_ntoa(p_c_addr->sin_addr),
           ntohs(p_c_addr->sin_port));

    CMD_TYPE e_cmd = (CMD_TYPE)msg->cmd;

    ssize_t sndn;

    socklen_t addrlen = sizeof(struct sockaddr_in);

    trans_msg msg_send;
    msg_send.dollar = '$';
    msg_send.cmd = -1;
    msg_send.text[0] = 'h';
    msg_send.text[1] = 'i';
    msg_send.port = 0;
    msg_send.addr = 0;

    size_t msg_size = sizeof(trans_msg);

    int ret;

    switch (e_cmd) {
        case S_C_DO_HOLE:

            printf("Start to Do Hole.\n");

            peer_addr.sin_port = msg->port;
            peer_addr.sin_addr.s_addr = msg->addr;

            msg_send.cmd = (int8_t)C_C_DO_HOLE_PACK;

            sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)&peer_addr, addrlen);
            if (sndn != msg_size) {
                printf("%s:%d Failed Send Hole Pack to Client 2.\n", __FILE__, __LINE__);
            }


            msg_send.cmd = (int8_t)C_S_DID_HOLE;

            sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)p_c_addr, addrlen);
            if (sndn != msg_size) {
                printf("%s:%d Failed Send to Server.\n", __FILE__, __LINE__);
            }

            if (0 != check_s_addr.sin_port) {
                sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *) &check_s_addr, addrlen);
                if (sndn != msg_size) {
                    printf("%s:%d Failed Send to Check Server.\n", __FILE__, __LINE__);
                }
            }

            ret = getsockname(s_fd, (struct sockaddr *)&temp_addr, &addrlen);
            if (ret < 0) {
                printf("%s:%d Failed to Get Sock Name, Errno: %d\n", __FILE__, __LINE__, errno);
            } else {
                printf("Now My IP:Port %s:%d.\n",
                       inet_ntoa(temp_addr.sin_addr), ntohs(temp_addr.sin_port));
            }

            printf("Did Hole to %s:%d.\n",
                   inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));

            break;

        case S_C_TRY_HOLE:

            printf("Start to Try Hole.\n");

            peer_addr.sin_port = msg->port;
            peer_addr.sin_addr.s_addr = msg->addr;

            msg_send.cmd = (int8_t)C_C_TRY_HOLE_PACK;


            sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)&peer_addr, addrlen);
            if (sndn != msg_size) {
                printf("%s:%d Failed Send Hole Pack to Client 1.\n", __FILE__, __LINE__);
            }

            msg_send.cmd = (int8_t)C_S_TRIED_HOLE;

            sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)p_c_addr, addrlen);
            if (sndn != msg_size) {
                printf("%s:%d Failed Send to Server.\n", __FILE__, __LINE__);
            }

            if (0 != check_s_addr.sin_port) {
                sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *) &check_s_addr, addrlen);
                if (sndn != msg_size) {
                    printf("%s:%d Failed Send to Check Server.\n", __FILE__, __LINE__);
                }
            }

            ret = getsockname(s_fd, (struct sockaddr *)&temp_addr, &addrlen);
            if (ret < 0) {
                printf("%s:%d Failed to Get Sock Name, Errno: %d\n", __FILE__, __LINE__, errno);
            } else {
                printf("Now My IP:Port %s:%d.\n",
                       inet_ntoa(temp_addr.sin_addr), ntohs(temp_addr.sin_port));
            }

            printf("Tried Hole to %s:%d.\n", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));

            break;

        case C_C_DO_HOLE_PACK:

            printf("Received Punch Hole Pack, My Network is Open?\n");

            break;

        case C_C_TRY_HOLE_PACK:

            printf("Received Try Hole Pack, Great.\n");

            msg_send.cmd = (int8_t)C_C_GOT_TRY_HOLE_PACK;

            sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)p_c_addr, addrlen);
            if (sndn != msg_size) {
                printf("%s:%d Failed Send to Peer.\n", __FILE__, __LINE__);
            }

            break;

        case C_C_GOT_TRY_HOLE_PACK:

            printf("Received Reply for my Try Hole Pack, Great.\n");

            msg_send.cmd = (int8_t)C_S_HOLE_SUS;

            sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)&s_addr, addrlen);
            if (sndn != msg_size) {
                printf("%s:%d Failed Send to Server.\n", __FILE__, __LINE__);
            }

            break;

        default:
            printf("Unknown Cmd. Client: %s:%d.\n",
                   inet_ntoa(p_c_addr->sin_addr), ntohs(p_c_addr->sin_port));
    }

    return 0;
}