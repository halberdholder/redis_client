
CC = gcc

LIB_SRCS = $(wildcard *.c)
LIB_OBJS = $(patsubst %.c, %.o, $(LIB_SRCS))
LIB_CFLAGS = -Wall -g -fpic
LIB_INCLUDES = -I../hiredis
LIB_LDFLAGS = -fpic -shared
LIB = libredis_client.so


%.o : %.c
	$(CC) $(LIB_CFLAGS) $(LIB_INCLUDES) -c $< -o $@

lib:$(LIB_OBJS)
	$(CC) $(LIB_LDFLAGS) -o $(LIB) $(LIB_OBJS)

clean:
	rm -rf $(LIB_OBJS) $(LIB)

.PHONY:lib clean

