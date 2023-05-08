CC = gcc

CFLAGS = -std=gnu99 -Wall -Wextra -g 

OBJS =  parse.o tcp_communication.o webserver.o 

all: webserver

webserver: $(OBJS)
	$(CC) $(CFLAGS) -o webserver $(OBJS)

webserver.o: webserver.c webserver.h
	$(CC) $(CFLAGS) -c webserver.c

parse.o: parse.c webserver.h
	$(CC) $(CFLAGS) -c parse.c

tcp_communication.o: tcp_communication.c webserver.h
	$(CC) $(CFLAGS) -c tcp_communication.c


clean:
	rm -f $(OBJS)

distclean:
	rm -f *~ *.o webserver $(OBJS)

