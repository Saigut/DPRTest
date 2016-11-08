#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <zconf.h>

#include "common.h"


struct sockaddr_in c1_addr;
struct sockaddr_in c2_addr;

volatile bool c1_rdy = false;
volatile bool c2_rdy = false;
volatile bool c2_failed = false;
volatile bool hole_succeed = false;

int deal_msg(int s_fd, trans_msg * msg, struct sockaddr_in * p_c_addr);
void * hole_thread(void * param);

int main( void ) {

    int ret;
    int s_fd;

    ret = setup_server(&s_fd, S_PORT, true);
    if (ret < 0) {
        printf("Failed to Set Up Server.\n");
    }

    memset(&c1_addr, 0, sizeof(c1_addr));
    memset(&c2_addr, 0, sizeof(c2_addr));

    pthread_t pid;
    ret = pthread_create(&pid, NULL, hole_thread, (void *)&s_fd);
    if (0 != ret) {
        printf("Failed to Create Thread.");
        return -1;
    }

    return start_server(s_fd, deal_msg);
}

void * hole_thread(void * param) {

    int s_fd = *((int *)param);

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

    while (1) {

        if (0 != c1_addr.sin_port && 0 != c2_addr.sin_port) {

            if (hole_succeed) {
                printf("Punch Hole Succeed!\n");
                break;
            }

            if (!c1_rdy) {

                msg_send.cmd = (int8_t)S_C_DO_HOLE;
                msg_send.port = c2_addr.sin_port;
                msg_send.addr = c2_addr.sin_addr.s_addr;
                sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)&c1_addr, addrlen);
                if (sndn != msg_size) {
                    printf("Failed Send to Client 1.\n");
                }
                printf("Told Client 1 to Punch Hole.\n");

            } else if (c2_failed) {

                printf("Punch Hole Failed.\n");

            } else if (c1_rdy && !c2_rdy) {

                msg_send.cmd = (int8_t)S_C_TRY_HOLE;
                msg_send.port = c1_addr.sin_port;
                msg_send.addr = c1_addr.sin_addr.s_addr;
                sndn = sendto(s_fd, &msg_send, msg_size, 0, (struct sockaddr *)&c2_addr, addrlen);
                if (sndn != msg_size) {
                    printf("Failed Send to Client 2.\n");
                }

                printf("Told Client 2 to Try Hole.\n");

            } else if (c1_rdy && c2_rdy) {

                c1_rdy = false;
                c2_rdy = false;
                printf("Punch Hole Again.\n");

            }

            usleep(500000);
        } else {
            sleep(1);
        }

    }

    return NULL;
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

    switch (e_cmd) {
        case C_S_HEART_BEAT:

            if (0 == c1_addr.sin_port) {
                c1_addr = *p_c_addr;
                printf("Client 1 added.\n");
            } else if (0 == c2_addr.sin_port) {
                if (p_c_addr->sin_addr.s_addr != c1_addr.sin_addr.s_addr
                    || p_c_addr->sin_port != c1_addr.sin_port) {
                    c2_addr = *p_c_addr;
                    printf("Client 2 added.\n");
                }
            }
            break;

        case C_S_DID_HOLE:

            if (p_c_addr->sin_addr.s_addr == c1_addr.sin_addr.s_addr
                && p_c_addr->sin_port == c1_addr.sin_port) {
                c1_rdy = true;
                printf("Client 1 Did Hole.\n");
            } else {
                printf("Wrong Client told me Did Hole. Client: %s:%d.\n",
                       inet_ntoa(p_c_addr->sin_addr), ntohs(p_c_addr->sin_port));
            }
            break;

        case C_S_TRIED_HOLE:

            if (p_c_addr->sin_addr.s_addr == c2_addr.sin_addr.s_addr
                && p_c_addr->sin_port == c2_addr.sin_port) {
                c2_rdy = true;
                printf("Client 2 Tried Hole.\n");
            } else {
                printf("Wrong Client told me Tried Hole. Client: %s:%d.\n",
                       inet_ntoa(p_c_addr->sin_addr), ntohs(p_c_addr->sin_port));
            }
            break;

        case C_S_HOLE_SUS:

            if (p_c_addr->sin_addr.s_addr == c2_addr.sin_addr.s_addr
                && p_c_addr->sin_port == c2_addr.sin_port) {
                hole_succeed = true;
                printf("Hole Opened. Great!\n");
            } else {
                printf("Wrong Client Tell me Hole Success. Client: %s:%d.\n",
                       inet_ntoa(p_c_addr->sin_addr), ntohs(p_c_addr->sin_port));
            }
            break;

        case C_S_HOLE_FAIL:
            if (p_c_addr->sin_addr.s_addr == c2_addr.sin_addr.s_addr
                && p_c_addr->sin_port == c2_addr.sin_port) {
                c2_failed = true;
                printf("Hole Failed.\n");
            } else {
                printf("Wrong Client Tell me Hole Fail. Client: %s:%d.\n",
                       inet_ntoa(p_c_addr->sin_addr), ntohs(p_c_addr->sin_port));
            }
            break;

        default:
            printf("Unknown Cmd. Client: %s:%d.\n",
                   inet_ntoa(p_c_addr->sin_addr), ntohs(p_c_addr->sin_port));
    }

    return 0;
}