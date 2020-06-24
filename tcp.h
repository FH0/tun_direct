#ifndef TCP_H
#define TCP_H

#include "main.h"
#include "redefine.h"

err_t tun_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
void handle_epoll_tcp(uint32_t events, tun_data_t *data);

#endif /* TCP_H */