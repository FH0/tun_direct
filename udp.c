#include "main.h"
#include "redefine.h"

/* udp_recv() callback */
void tun_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                  const ip_addr_t *saddr, u16_t sport, const ip_addr_t *daddr,
                  u16_t dport) {
  ELOG(__FUNCTION__);

  tun_data_t *data;
  static char *buf = NULL;
  int bufLen;

  printf("%d %d\n", sport, dport);

  /* get buf form pbuf */
  if (!buf) {
    buf = MALLOC(MTU);
  }
  if (p->tot_len >= MTU) {
    ELOG("big udp packet");
    return;
  }
  bufLen = pbuf_copy_partial(p, buf, p->tot_len, 0);
  if (bufLen == 0) {
    ELOG("pbuf_copy_partial()");
    return;
  }
  pbuf_free(p);

  if (saddr->type != IPADDR_TYPE_V4 && saddr->type != IPADDR_TYPE_V6) {
    ELOG("unsupport address type");
    return;
  }

  /* filled struct and sendto */
  data = MALLOC(sizeof(tun_data_t));
  data->upcb = upcb;
  if (saddr->type == IPADDR_TYPE_V4) {
    struct sockaddr_in sai;

    sai.sin_family = AF_INET;
    memcpy(&sai.sin_addr, &daddr->u_addr, 4);
    sai.sin_port = htons(dport);

    data->type = TUN_UDP;
    memcpy(data->saddr, &saddr->u_addr, 4);
    memcpy(data->daddr, &daddr->u_addr, 4);
    data->sport = sport;
    data->dport = dport;

    data->fd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT(data->fd != -1);
    epoll_ET_add(data, epollfd, EPOLLIN);

    sendto(data->fd, buf, bufLen, 0, (struct sockaddr *)&sai, sizeof(sai));
  } else if (saddr->type == IPADDR_TYPE_V6) {
    struct sockaddr_in6 sai;

    sai.sin6_family = AF_INET6;
    memcpy(&sai.sin6_addr, &daddr->u_addr, 16);
    sai.sin6_port = htons(dport);

    data->type = TUN_UDP6;
    memcpy(data->saddr, &saddr->u_addr, 16);
    memcpy(data->daddr, &daddr->u_addr, 16);
    data->sport = sport;
    data->dport = dport;

    data->fd = socket(AF_INET6, SOCK_DGRAM, 0);
    ASSERT(data->fd != -1);
    epoll_ET_add(data, epollfd, EPOLLIN);

    sendto(data->fd, buf, bufLen, 0, (struct sockaddr *)&sai, sizeof(sai));
  }
}

void handle_epoll_udp(uint32_t events, tun_data_t *data) {
  ELOG(__FUNCTION__);

  static char *buf = NULL;
  int nread;
  ip_addr_t saddr, daddr;
  struct pbuf *p;

  printf("%d %d\n", data->sport, data->dport);

  /* get data from udp socket fd */
  if (!buf) {
    buf = MALLOC(MTU);
  }
  nread = read(data->fd, buf, MTU);
  if (nread <= 0 || nread == MTU) {
    ELOG("error udp packet size");
    return;
  }

  /* destination information */
  if (data->type == TUN_UDP) {
    saddr.type = IPADDR_TYPE_V4;
    memcpy(&saddr.u_addr, &data->saddr, 4);
    daddr.type = IPADDR_TYPE_V4;
    memcpy(&daddr.u_addr, &data->daddr, 4);
  } else if (data->type == TUN_UDP6) {
    saddr.type = IPADDR_TYPE_V6;
    memcpy(&saddr.u_addr, &data->saddr, 16);
    daddr.type = IPADDR_TYPE_V6;
    memcpy(&daddr.u_addr, &data->daddr, 16);
  }

  /* pack data and sendto */
  p = pbuf_alloc_reference(buf, nread, PBUF_ROM);
  if (!p) {
    ELOG("pbuf_alloc_reference");
    return;
  }

  udp_sendto(data->upcb, p, &saddr, data->sport, &daddr, data->dport);

  close(data->fd);
  FREE(data);
}
