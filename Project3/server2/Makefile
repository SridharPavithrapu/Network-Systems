INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= dfs2.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	dfs2

clean:
	-rm -f *.o *.d
	-rm -f dfs2

feasibility_tests: client.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lm

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
