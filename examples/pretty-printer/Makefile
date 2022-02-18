NANOCBOR_DIR = ../..

include ../../Makefile

SRCS += main.c

bin/pretty-printer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

test : CFLAGS += -g3

pretty-printer: bin/pretty-printer
