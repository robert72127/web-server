CC = gcc

CFLAGS = -std=gnu99 -Wall -Wextra -g 

OBJS =  webserver.o 

all: webserver

webserver: $(OBJS)
	$(CC) $(CFLAGS) -o webserver $(OBJS)

webserver.o: transport.c transport.h
	$(CC) $(CFLAGS) -c webserver.c


clean:
	rm -f $(OBJS)

distclean:
	rm -f *~ *.o webserver

