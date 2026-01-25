CC=gcc

CFLAGS=-g -Wall -finstrument-functions -pthread

LDFLAGS=-L../hpc-lib/ -rdynamic
LDLIBS=-lhpc

SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)

TGTS=$(SRCS:%.c=%)

all: $(TGTS)

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RM) $(TGTS) $(OBJS)

.phony: clean all
