#include "redefine.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/ipv6.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* return fd value on success */
int create_tun(char *path, char *name, int mtu, char *addr, char *netmask,
               char *addr6, int prefixlen) {
  struct sockaddr_in sai;
  int fd, sockfd, sock6fd;
  struct ifreq ifr;
  struct in6_ifreq ifr6;

  /* get fd */
  fd = open(path, O_RDWR);
  ASSERT(fd != -1);

  /* set name */
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  strncpy(ifr.ifr_name, name, IFNAMSIZ);
  ASSERT(ioctl(fd, TUNSETIFF, &ifr) != -1);

  /* socket to connect kernel */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  ASSERT(sockfd != -1);

  /* set addr */
  memset(&sai, 0, sizeof(sai));
  sai.sin_family = AF_INET;
  inet_pton(AF_INET, addr, &sai.sin_addr.s_addr);
  memcpy(&ifr.ifr_addr, &sai, sizeof(sai));
  ASSERT(ioctl(sockfd, SIOCSIFADDR, &ifr) != -1);

  /* set netmask */
  inet_pton(AF_INET, netmask, &sai.sin_addr);
  memcpy(&ifr.ifr_addr, &sai, sizeof(sai));
  ASSERT(ioctl(sockfd, SIOCSIFNETMASK, &ifr) != -1);

  /* ipv6 */
  if (addr6 && prefixlen) {
    /* socket to connect kernel */
    sock6fd = socket(AF_INET6, SOCK_DGRAM, 0);
    ASSERT(sock6fd != -1);

    /* address and prefixlen*/
    inet_pton(AF_INET6, addr6, &ifr6.ifr6_addr);
    ifr6.ifr6_prefixlen = prefixlen;
    ASSERT(-1 != (ioctl(sock6fd, SIOGIFINDEX, &ifr)));
    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    ASSERT(ioctl(sock6fd, SIOCSIFADDR, &ifr6) != -1);

    close(sock6fd);
  }

  /* set mtu */
  ifr.ifr_mtu = mtu;
  ASSERT(ioctl(sockfd, SIOCSIFMTU, &ifr) != -1);

  /* start device */
  ifr.ifr_flags |= IFF_UP;
  ASSERT(ioctl(sockfd, SIOCSIFFLAGS, &ifr) != -1);

  close(sockfd);
  return fd;
}

// int main(int argc, char const *argv[]) {
//   create_tun("/dev/net/tun", "tun3", 1500, "10.5.5.5", "255.255.255.0",
//              "fd55::5", 64);
//   sleep(100);

//   return 0;
// }
