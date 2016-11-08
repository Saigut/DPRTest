#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"


int deal_msg(int s_fd, trans_msg * msg, struct sockaddr_in * p_c_addr);
void * hole_thread(void * param);

int main( void ) {

    int ret;
    int s_fd;

    ret = setup_server(&s_fd, CHECK_S_PORT, true);
    if (ret < 0) {
        printf("Failed to Set Up Check Server.\n");
    }


    return start_server(s_fd, deal_msg);
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

    return 0;
}