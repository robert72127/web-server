#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

#include "webserver.h"

/* via beej guide, and CSAPP */
int open_listenfd(char *port){
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;
    /* ... using port number */
    getaddrinfo(NULL, port, &hints, &listp);

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */
        
        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
        (const void *)&optval , sizeof(int));

        /* Bind the descriptor to the address */
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; /* Success */
        close(listenfd); /* Bind failed, try the next */    
    }
     /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, BACKLOG) < 0) {
        close(listenfd);
        return -1;
    }
    return listenfd;
}

ssize_t read_tcp (int fd, int waittime, char *buffer){
    struct timeval tv; tv.tv_sec = waittime; tv.tv_usec = 0;
    size_t total_bytes_read = 0;
    ssize_t bytes_read;
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(fd, &descriptors);
    while (total_bytes_read < BUFFER_SIZE) {

        int ready = select(fd + 1, &descriptors, NULL, NULL, &tv);
        if (ready < 0){
            ERROR("select error");
        }
        if (ready == 0){
            return 0;
        }

        bytes_read = recv(fd, buffer + total_bytes_read, BUFFER_SIZE - total_bytes_read, 0);
        if (bytes_read < 0)
            return 0;
            //ERROR("recv error");
        if (bytes_read == 0){
            /* didnt read anything will close connection right aways*/
            return 0;
        }


        total_bytes_read += bytes_read;
        if (total_bytes_read > 3 && buffer[total_bytes_read-3] == 10 && buffer[total_bytes_read-2] == 13 && buffer[total_bytes_read-1] == 10){
            buffer[total_bytes_read-2] = '\0';
            break;
        }
    }
    return 1;
}

/* write response to assoctiated fd*/
void respond(int fd, char *resp_buffer, int buff_size){
    /*might need to handle if cant send all at once*/
    buff_size = (buff_size == 0)? strlen(resp_buffer) : buff_size;
    int bytes_sent = 0;
    int size;

    while(bytes_sent < buff_size){
        size = (buff_size < MAXCHUNK)? buff_size : MAXCHUNK;

        bytes_sent += send(fd, resp_buffer+bytes_sent, size, 0);
    }
    if(bytes_sent == -1)
        perror("send error\n");
}