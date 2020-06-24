#include "main.h"
#include "init_tun.h"
#include "redefine.h"
#include "tcp.h"
#include "udp.h"

int tunfd, epollfd;

static int setnonblocking(int fd);
static err_t output(struct pbuf *p);
static err_t output4(struct netif *netif, struct pbuf *p,
                     const ip4_addr_t *ipaddr);
static err_t output6(struct netif *netif, struct pbuf *p,
                     const ip6_addr_t *ipaddr);
static void hex_dump(char *buf, int len);

int main(int argc, char const *argv[]) {
  int nfds, n, nread;
  struct epoll_event events[EPOLL_SIZE];
  struct tcp_pcb *tpcb;
  struct udp_pcb *upcb;
  struct pbuf *p;
  char *buf;
  tun_data_t *data;
  err_t err;
  extern int errno;

  /* do not exit when socket broken */
  signal(SIGPIPE, SIG_IGN);

  /* lwip */
  lwip_init();
  netif_list->output = output4;
  netif_list->output_ip6 = output6;
  ASSERT(tpcb = tcp_new());
  err = tcp_bind(tpcb, IP_ANY_TYPE, 0);
  ASSERT(err == ERR_OK);
  tpcb = tcp_listen(tpcb);
  tcp_accept(tpcb, tun_tcp_accept);
  ASSERT(upcb = udp_new());
  err = udp_bind(upcb, IP_ANY_TYPE, 0);
  ASSERT(err == ERR_OK);
  udp_recv(upcb, tun_udp_recv, NULL);

  /* epoll */
  epollfd = epoll_create1(0);

  /* tun device */
  tunfd = create_tun(TUN_PATH, "tun3", MTU, "10.5.5.5", "255.255.255.0",
                     "fd55::5", 64);
  data = MALLOC(sizeof(tun_data_t));
  data->fd = tunfd;
  epoll_ET_add(data, epollfd, EPOLLIN);

  /* init buf to store data from tunfd */
  buf = MALLOC(MTU);

  /* loop */
  ELOG("entry loop");
  for (;;) {
    nfds = epoll_wait(epollfd, events, EPOLL_SIZE, 20);
    for (n = 0; n < nfds; n++) {
      data = (tun_data_t *)events[n].data.ptr;

      /* use lwip to handle ip data */
      if (data->fd == tunfd) {
        for (;;) {
          nread = read(tunfd, buf, MTU);
          if (nread == -1) {
            ASSERT(errno == EAGAIN);
            break;
          }
          ASSERT(nread != MTU);

          hex_dump(buf, nread);
          p = pbuf_alloc(PBUF_RAW, nread, PBUF_POOL);
          ASSERT(p);
          pbuf_take(p, buf, nread);
          err = netif_list->input(p, netif_list);
          ASSERT(err == ERR_OK);
        }
      }
      /* use lwip to generate ip data  */
      else if (events[n].events & EPOLLIN || events[n].events & EPOLLOUT) {
        if (data->type == TUN_TCP) {
          handle_epoll_tcp(events[n].events, data);
        } else if (data->type == TUN_UDP || data->type == TUN_UDP6) {
          handle_epoll_udp(events[n].events, data);
        } else {
          ELOG("unkown type");
          continue;
        }
      }
      /* something unexpected happened */
      else {
        ELOG("epoll events");
        continue;
      }
    }
  }

  FREE(buf);
  return 0;
}

static err_t output4(struct netif *netif, struct pbuf *p,
                     const ip4_addr_t *ipaddr) {
  return output(p);
}

static err_t output6(struct netif *netif, struct pbuf *p,
                     const ip6_addr_t *ipaddr) {
  return output(p);
}

/* for lwip netif output */
static err_t output(struct pbuf *p) {
  static char buf[MTU];
  int nwrite, bufLen;

  bufLen = pbuf_copy_partial(p, buf, p->tot_len, 0);
  nwrite = write(tunfd, buf, bufLen);
  if (nwrite == -1) {
    ELOG("output()");
  }
  return ERR_OK;
}

/* set fd non-blocking */
static int setnonblocking(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

/* add fd to epoll events */
void epoll_ET_add(tun_data_t *data, int epollfd, u32_t events) {
  struct epoll_event event;

  if (setnonblocking(data->fd) == -1) {
    ELOG("setnonblocking()");
    return;
  }
  event.data.ptr = data;
  event.events = events | EPOLLET;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, data->fd, &event) == -1) {
    ELOG("epoll_ctl()");
    return;
  }
}

/* modify epoll events */
void epoll_ET_mod(tun_data_t *data, int epollfd, u32_t events) {
  struct epoll_event event;

  event.data.ptr = data;
  event.events = events | EPOLLET;
  if (epoll_ctl(epollfd, EPOLL_CTL_MOD, data->fd, &event) == -1) {
    ELOG("epoll_ctl()");
    return;
  }
}

static void hex_dump(char *buf, int len) {
  int i;

  for (i = 0; i < len; i++) {
    printf("%02X ", (uint8_t)buf[i]);

    if (((i + 1) % 16) == 0) {
      printf("\n");
    } else if (((i + 1) % 8) == 0) {
      printf(" ");
    }
  }

  if ((len % 16) != 0) {
    printf("\n");
  }
  printf("\n");
}