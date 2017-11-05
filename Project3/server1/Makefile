INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= dfs1.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	dfs1

clean:
	-rm -f *.o *.d
	-rm -f dfs1

feasibility_tests: client.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lm

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
