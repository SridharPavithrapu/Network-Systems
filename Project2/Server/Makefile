INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CFLAGS= -O0 -g $(INCLUDE_DIRS)
LIBS= 

HFILES= 
CFILES= server.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	server

clean:
	-rm -f *.o *.d
	-rm -f server

%.o: %.c $(SRCS)
	$(CC) -c -o $@ $< $(CFLAGS)

