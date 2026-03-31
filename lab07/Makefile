CC=gcc

CFLAGS=-fPIC -fopenmp

LDFLAGS=
LDLIBS=

SRCS=$(wildcard *.c)
OBJS=$(SRCS:%.c=%.o)

TGTS=$(SRCS:%.c=%)
SOS=$(SRCS:%.c=%.so)

all: $(TGTS) $(SOS)

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

%.so: %.c
	$(CC) $(CFLAGS) -shared $< -o $@ $(LDLIBS)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RM) $(TGTS) $(OBJS) $(SOS)

.phony: clean all
