CC = gcc
CFLAGS = -Wall -g3

INSTALL_PATH = $$HOME/simgrid-3.14.159
INCLUDES = -I../include -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib
LDADD = -lm -lsimgrid

SRC=$(wildcard *.c)
BIN=$(SRC:.c=.bin)

all: $(BIN)

%.bin: %.c
	$(CC) $(INCLUDES) $(DEFS) -o $@ $^ ../libmra.a $(LDADD)

clean:
	rm -vf *.bin *.trace *.plist *.log *.csv

.PHONY: clean
