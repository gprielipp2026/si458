CC=gcc
CFLAGS=-Wall

SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)

TGT=array

all: $(TGT)

$(TGT): $(OBJS)
	$(CC) $(CFLAGS) $< -o $@

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RM) $(TGT) $(OBJS)

.phony: clean all
