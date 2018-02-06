
CC = gcc

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
CFLAGS = -Wall -g
INCLUDES = -Ihiredis -Iredisclient
LIBDIR = redisclient
LDFLAGS = -Wl,-Bdynamic -Lredisclient -lredis_client -lpthread -Lhiredis -lhiredis -llua -Wl,--rpath=./$(LIBDIR)

TARGET = demo


%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

all:$(OBJS)
	make -C $(LIBDIR)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(OBJS) $(TARGET)
	make -C $(LIBDIR) clean
