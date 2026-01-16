CC=gcc
CFLAGS=-g -Wall -finstrument-functions 

LDFLAGS=-L/usr/local/include/hpc-lib/timing/ -rdynamic
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
