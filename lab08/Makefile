CC=nvcc

CFLAGS=

LDFLAGS=
LDLIBS=

SRCS=$(wildcard *.cu)

TGTS=$(SRCS:%.cu=%)

all: $(TGTS) 

%: %.cu
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ $(LDLIBS)

clean:
	$(RM) $(RM) $(TGTS) 

.phony: clean all
