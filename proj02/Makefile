CC=gcc

CFLAGS=-Wall 

LDFLAGS=
LDLIBS=

SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)

TGTS=$(SRCS:%.c=%)

all: $(TGTS)

%: %.c
	mpicc $(CFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

clean:
	$(RM) $(RM) $(TGTS) $(OBJS)

.phony: clean all
