#ifndef DPRTEST_COMMON_H
#define DPRTEST_COMMON_H

#include <stdint.h>

typedef enum _CMD_TYPE {
    C_S_HEART_BEAT = 0,
    C_S_DID_HOLE,
    C_S_TRIED_HOLE,
    C_S_HOLE_SUS,
    C_S_HOLE_FAIL,

    S_C_DO_HOLE = 10,
    S_C_TRY_HOLE,

    C_C_DO_HOLE_PACK = 20,
    C_C_TRY_HOLE_PACK,
    C_C_GOT_TRY_HOLE_PACK
} CMD_TYPE;


#pragma  pack(push, 1)
typedef struct {
    char dollar;
    int8_t cmd;
    char text[2];
    uint16_t port;          // network byte order
    uint32_t addr;          // network byte order
} trans_msg;
#pragma  pack(pop)

#define S_PORT  60001
#define CHECK_S_PORT  61001


typedef int (*start_server_cb)(int s_fd, trans_msg * msg, struct sockaddr_in * p_c_addr);

int setup_server(int *p_s_fd, uint16_t s_port, bool port_reuse);
int start_server(int s_fd, start_server_cb p_cb);

#endif //DPRTEST_COMMON_H
