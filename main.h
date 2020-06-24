#ifndef MAIN_H
#define MAIN_H

#include "lwip-2.1.2/src/include/lwip/init.h"
#include "lwip-2.1.2/src/include/lwip/ip_addr.h"
#include "lwip-2.1.2/src/include/lwip/netif.h"
#include "lwip-2.1.2/src/include/lwip/pbuf.h"
#include "lwip-2.1.2/src/include/lwip/tcp.h"
#include "lwip-2.1.2/src/include/lwip/udp.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define EPOLL_SIZE 512
#define MTU 1500
#define TUN_TCP_SIZE 65535

#ifdef __ANDROID__
#define TUN_PATH "/dev/tun"
#else
#define TUN_PATH "/dev/net/tun"
#endif

enum {
  TUN_CONNECTING = 0,
  TUN_CONNECTED,
  TUN_CLOSE,
  TUN_TCP,
  TUN_UDP,
  TUN_UDP6
};

typedef struct tun_data {
  int type; /* tcp or udp */
  int fd;   /* socket fd */
  union {
    struct { /* tcp */
      struct tcp_pcb *tpcb;
      int client_status;
      int server_status;
      char *client_buf;
      char *server_buf;
      int client_buf_len;
      int server_buf_len;
    };
    struct { /* udp */
      struct udp_pcb *upcb;
      char saddr[16];
      char daddr[16];
      uint16_t sport;
      uint16_t dport;
    };
  };
} tun_data_t;

void epoll_ET_add(tun_data_t *data, int epollfd, u32_t events);
void epoll_ET_mod(tun_data_t *data, int epollfd, u32_t events);

extern int tunfd, epollfd;

#endif /* MAIN_H */