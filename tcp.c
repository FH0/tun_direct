#include "tcp.h"

static err_t tun_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                          err_t err);
static void tun_tcp_error(void *arg, err_t err);
static err_t tun_tcp_send(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tun_tcp_free(tun_data_t *data);
static void tun_tcp_close(tun_data_t *data);
static void lwip_to_epoll(tun_data_t *data);
static void epoll_to_lwip(tun_data_t *data);

err_t tun_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
  ELOG(__FUNCTION__);

  if (err != ERR_OK || newpcb == NULL) {
    ELOG("err != ERR_OK || newpcb == NULL");
    return ERR_VAL;
  }

  if (newpcb->local_ip.type != IPADDR_TYPE_V4 &&
      newpcb->local_ip.type != IPADDR_TYPE_V6) {
    ELOG("unsupport ip type");
    tcp_close(newpcb);
  }

  tun_data_t *data;
  extern int errno;

  /* filled struct */
  data = MALLOC(sizeof(tun_data_t));
  data->type = TUN_TCP;
  data->tpcb = newpcb;
  data->client_buf = MALLOC(TUN_TCP_SIZE);
  data->server_buf = MALLOC(TUN_TCP_SIZE);
  data->client_buf_len = 0;
  data->server_buf_len = 0;
  data->type = TUN_TCP;
  data->client_status = TUN_CONNECTED;
  data->server_status = TUN_CONNECTING;

  /* add event to epoll */
  if (newpcb->local_ip.type == IPADDR_TYPE_V4) {
    struct sockaddr_in sai;

    sai.sin_family = AF_INET;
    memcpy(&sai.sin_addr, &newpcb->local_ip.u_addr, 4);
    sai.sin_port = htons(newpcb->local_port);

    data->fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(data->fd != -1);
    epoll_ET_add(data, epollfd, EPOLLIN | EPOLLOUT);
    if (connect(data->fd, (struct sockaddr *)&sai, sizeof(sai)) == -1 &&
        errno != EINPROGRESS) {
      ELOG("connect()");
      tun_tcp_close(data);
    }
  } else if (newpcb->local_ip.type == IPADDR_TYPE_V6) {
    struct sockaddr_in6 sai;

    sai.sin6_family = AF_INET6;
    memcpy(&sai.sin6_addr, &newpcb->local_ip.u_addr, 16);
    sai.sin6_port = htons(newpcb->local_port);

    data->fd = socket(AF_INET6, SOCK_STREAM, 0);
    ASSERT(data->fd != -1);
    epoll_ET_add(data, epollfd, EPOLLIN | EPOLLOUT);
    if (connect(data->fd, (struct sockaddr *)&sai, sizeof(sai)) == -1 &&
        errno != EINPROGRESS) {
      ELOG("connect()");
      tun_tcp_close(data);
    }
  }

  /* lwip callback */
  tcp_arg(newpcb, data);
  tcp_err(newpcb, tun_tcp_error);
  tcp_recv(newpcb, tun_tcp_recv);
  tcp_sent(newpcb, tun_tcp_send);

  return ERR_OK;
}

static err_t tun_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                          err_t err) {
  ELOG(__FUNCTION__);

  ASSERT(err == ERR_OK);

  tun_data_t *data;
  int n;

  data = (tun_data_t *)arg;

  /* closed, peer will send no data */
  if (!p) {
    ELOG("client closed");

    if (data->server_status == TUN_CLOSE) {
      tun_tcp_close(data);
      return ERR_OK;
    } else {
      data->client_status = TUN_CLOSE;
    }
  }
  /* save pbuf to data->client_buf */
  else {
    if ((p->tot_len + data->client_buf_len) > TUN_TCP_SIZE) {
      ELOG("out of TUN_TCP_SIZE");
      tun_tcp_close(data);
      return ERR_OK;
    }

    n = pbuf_copy_partial(p, data->client_buf + data->client_buf_len,
                          p->tot_len, 0);
    if (n != p->tot_len) {
      ELOG("pbuf_copy_partial()");
      tun_tcp_close(data);
      return ERR_OK;
    }
    data->client_buf_len += n;
    pbuf_free(p);
  }

  /* can't do anything if server is TUN_CONNECTING */
  if (data->server_status == TUN_CONNECTED ||
      data->server_status == TUN_CLOSE) {
    lwip_to_epoll(data);
  }

  return ERR_OK;
}

static void tun_tcp_error(void *arg, err_t err) {
  ELOG(__FUNCTION__);
  printf("err %d\n", err);

  close(((tun_data_t *)arg)->fd);
  tun_tcp_free((tun_data_t *)arg);
}

/* client writable callback */
static err_t tun_tcp_send(void *arg, struct tcp_pcb *tpcb, u16_t len) {
  ELOG(__FUNCTION__);

  epoll_to_lwip((tun_data_t *)arg);

  return ERR_OK;
}

void handle_epoll_tcp(uint32_t events, tun_data_t *data) {
  ELOG(__FUNCTION__);

  if (data->server_status == TUN_CONNECTING) {
    data->server_status = TUN_CONNECTED;
  }
  if (events & EPOLLIN) {
    epoll_to_lwip(data);
  } else {
    lwip_to_epoll(data);
  }
}

static void tun_tcp_free(tun_data_t *data) {
  ELOG(__FUNCTION__);

  FREE(data->client_buf);
  FREE(data->server_buf);
  FREE(data);
}

static void tun_tcp_close(tun_data_t *data) {
  ELOG(__FUNCTION__);

  tcp_arg(data->tpcb, NULL);
  tcp_err(data->tpcb, NULL);
  tcp_recv(data->tpcb, NULL);
  tcp_sent(data->tpcb, NULL);

  tcp_close(data->tpcb);
  close(data->fd);
  tun_tcp_free(data);
}

/* send data from lwip to epoll */
static void lwip_to_epoll(tun_data_t *data) {
  ELOG(__FUNCTION__);

  int nwrite;
  extern int errno;

  /* exit if sending is completed or unable to send */
  nwrite = write(data->fd, data->client_buf, data->client_buf_len);
  if (nwrite == -1 && errno != EAGAIN) {
    ELOG("write()");
    tun_tcp_close(data);
    return;
  } else if (nwrite > 0) {
    tcp_recved(data->tpcb, nwrite);
    data->client_buf_len -= nwrite;
    memmove(data->client_buf + nwrite, data->client_buf, data->client_buf_len);
  }

  /* TUN_CLOSE and no buf left, should shutdown write */
  if (data->client_buf_len == 0 && data->client_status == TUN_CLOSE) {
    ELOG("shutdown clientserver write");

    epoll_ET_mod(data, epollfd,
                 EPOLLIN); /* without this step epoll will loop indefinitely */
    shutdown(data->fd, SHUT_WR);
  }
}

/* send data from epoll to lwip */
static void epoll_to_lwip(tun_data_t *data) {
  ELOG(__FUNCTION__);

  int nread, send_len;
  extern int errno;
  err_t err;

  nread = read(data->fd, data->server_buf + data->server_buf_len,
               TUN_TCP_SIZE - data->server_buf_len);
  if (nread == -1 && errno != EAGAIN) {
    ELOG("read()");
    tun_tcp_close(data);
    return;
  } else if (nread == 0) {
    if (data->client_status == TUN_CLOSE) {
      tun_tcp_close(data);
      return;
    } else {
      data->server_status = TUN_CLOSE;
    }
  } else if (nread > 0) {
    data->server_buf_len += nread;

    send_len = (data->server_buf_len < tcp_sndbuf(data->tpcb))
                   ? data->server_buf_len
                   : tcp_sndbuf(data->tpcb);

    err =
        tcp_write(data->tpcb, data->server_buf, send_len, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
      data->server_buf_len -= send_len;
      tcp_output(data->tpcb);
    } else if (err != ERR_MEM) {
      ELOG("tcp_write()");
      tun_tcp_close(data);
      return;
    }
  }

  /* TUN_CLOSE and no buf left, should shutdown write side */
  if (data->server_buf_len == 0 && data->server_status == TUN_CLOSE) {
    ELOG("shutdown server write");

    tcp_shutdown(data->tpcb, 0, 1);
  }
}
