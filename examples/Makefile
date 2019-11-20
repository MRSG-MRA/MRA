CC = g++ -std=c++11
CFLAGS = -Wall -g3

INSTALL_PATH = $$HOME/SimGrid-3.23.2
INCLUDES = -I../include -I$(INSTALL_PATH)/include 
DEFS = -L$(INSTALL_PATH)/lib
LDADD = -lm -lsimgrid

SRC=$(wildcard *.cpp)
BIN=$(SRC:.c=.bin)

#all: $(BIN)
all: hello_mra.bin

hello_mra.bin: hello_mra.cpp
	$(CC) $(INCLUDES) $(DEFS) -o $@ $^ ../libmra.a $(LDADD)

#%.bin: %.cpp
#	$(CC) $(INCLUDES) $(DEFS) -o $@ $^ ../libmrsg.a $(LDADD)

clean:
	rm -vf *.bin *.csv *.trace *.plist *.log

.PHONY: clean
