# Generate the scopedaq and scopetalk program
#CFLAGS = -Wall -g
CC = g++

# OS name
UNAME := $(shell uname -s)
# OS independent flags
VXI11_LIB=

# MacOS (OSX) specific flags
ifeq ($(UNAME),Darwin)
  VXI11_LIB=-L/usr/local/lib
endif

# Linux specific flags
ifeq ($(UNAME),Linux)
  VXI11_LIB=-L/usr/local/lib
endif


all: clean talkscope scopedaq

talkscope: talkscope.cpp 
	$(CC) $(CCFLAGS) talkscope.cpp  -lvxi11 -o talkscope

scopedaq: scopedaq.cpp 
	$(CC) $(CCFLAGS) scopedaq.cpp  -lvxi11 -o scopedaq
	


clean:
	rm -f scopedaq talkscope *.o  *~

