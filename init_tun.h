#ifndef  INIT_TUN_H
#define INIT_TUN_H

int create_tun(char *path, char *name, int mtu, char *addr, char *netmask,
               char *addr6, int prefixlen);

#endif /* INIT_TUN_H */