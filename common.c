//
// Created by saigut on 11/1/16.
//
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "common.h"

int setup_server(int *p_s_fd, uint16_t s_port, bool port_reuse) {

    int ret;
    int s_fd;
    struct sockaddr_in s_addr;

    s_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_fd < 0) {
        printf("Failed to create socket, Errno: %d\n", errno);
        return -1;
    }

    if (port_reuse) {
        int on = 1;
        ret = setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (ret < 0) {
            printf("Failed to Set Socket. Errno: %d\n", errno);
            close(s_fd);
            return -1;
        }
    }


    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    s_addr.sin_port = htons(s_port);

    socklen_t addrlen = sizeof(struct sockaddr_in);

    ret = bind(s_fd, (struct sockaddr *)&s_addr, addrlen);
    if (ret < 0) {
        printf("Failed to bind socket, Errno: %d\n", errno);
        close(s_fd);
        return -1;
    }

    ret = getsockname(s_fd, (struct sockaddr *)&s_addr, &addrlen);
    if (ret < 0) {
        printf("Failed to Get Sock Name, Errno: %d\n", errno);
        close(s_fd);
        return -1;
    }

    printf("Bound to %d\n", ntohs(s_addr.sin_port));


    *p_s_fd = s_fd;

    return 0;
}

int start_server(int s_fd, start_server_cb p_cb) {

    if (!p_cb) {
        printf("Server Callback is Not Provided.\n");
    }

    int ret;
    struct sockaddr_in c_addr;
    fd_set RFDs;

    ssize_t recvn = 0;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    trans_msg msg_recvd;
    size_t msg_size = sizeof(trans_msg);

    while (1) {

        FD_ZERO(&RFDs);
        FD_SET(s_fd, &RFDs);

        ret = select(s_fd + 1, &RFDs, NULL, NULL, NULL);
        if (ret > 0) {

            recvn = recvfrom(s_fd, &msg_recvd, msg_size, 0, (struct sockaddr *)&c_addr, &addrlen);

            if (recvn == msg_size) {

                if ('$' == msg_recvd.dollar) {
                    if (p_cb) {
                        p_cb(s_fd, &msg_recvd, &c_addr);
                    }
                } else {
                    printf("Malformed Massage.\n");
                }

            } else if (recvn < 0) {
                printf("Failed to Recieve. Errno: %d\n", errno);
            } else if (0 == recvn) {
                printf("Closed by Peer.\n");
            } else {
                printf("Wrong Message Size. Expect %ld, Recieved %ld\n", msg_size, recvn);
            }

        } else if (-1 == ret) {

            printf("Select Error, Errno: %d\n", errno);
            return -1;

        }
    }
}