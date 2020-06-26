CC = gcc
override CFLAGS += -I./lwip-2.1.2/src/include -I./lwip-2.1.2/src/custom -Wall -g -ggdb
LWIP_SRC = $(wildcard lwip-2.1.2/src/core/*.c lwip-2.1.2/src/core/ipv4/*.c lwip-2.1.2/src/core/ipv6/*.c) lwip-2.1.2/src/custom/sys_arch.c
LWIP_OBJ = $(patsubst %.c,%.o,$(LWIP_SRC))
TUN_DIRECT_SRC = $(wildcard *.c)
TUN_DIRECT_OBJ = $(patsubst %.c,%.o,$(TUN_DIRECT_SRC))

all: lwip $(TUN_DIRECT_OBJ)
	$(CC) $(CFLAGS) -o tun_direct $(TUN_DIRECT_OBJ) liblwip.a

lwip: $(LWIP_OBJ)
	ar -r -s liblwip.a $(LWIP_OBJ)

clean:
	-rm -rf $(LWIP_OBJ) $(TUN_DIRECT_OBJ) liblwip.a tun_direct
