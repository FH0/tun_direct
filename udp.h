#ifndef UDP_H
#define UDP_H

void handle_epoll_udp(uint32_t events, tun_data_t *data);
void tun_udp_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                  const ip_addr_t *saddr, u16_t sport, const ip_addr_t *daddr,
                  u16_t dport);

#endif /* UDP_H */