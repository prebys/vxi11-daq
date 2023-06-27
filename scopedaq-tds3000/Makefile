# Generate the scopedaq and scopetalk program
#CFLAGS = -Wall -g
CC = gcc
RPCGEN = rpcgen

all: clean talkscope scopedaq

talkscope: talkscope.c vxi11.h
	$(CC) -o talkscope talkscope.c
	
scopedaq: scopedaq.c vxi11.h
	$(CC) -o scopedaq scopedaq.c

vxi11.h: vxi11.rpcl
	$(RPCGEN) vxi11.rpcl

clean:
	rm -f scopedaq talkscope *.o vxi11.h vxi11_svc.c vxi11_xdr.c vxi11_clnt.c *~

