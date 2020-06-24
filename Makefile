CC = gcc
CFLAGS += -I./lwip-2.1.2/src/include -I./lwip-2.1.2/src/custom -Wall -g -ggdb
SRC = $(wildcard *.c ./lwip-2.1.2/src/core/*.c ./lwip-2.1.2/src/core/ipv4/*.c ./lwip-2.1.2/src/core/ipv6/*.c) ./lwip-2.1.2/src/custom/sys_arch.c
OBJ = $(patsubst %.c,%.o,$(SRC))

all: $(OBJ)
	$(CC) $(CFLAGS) -o tun_direct $(OBJ)

clean:
	-rm -rf $(OBJ) tun_direct
