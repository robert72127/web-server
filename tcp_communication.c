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

    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(fd, &descriptors);
    while (total_bytes_read < BUFFER_SIZE) {
       // printf("DEBUG: Current value of tv = %.3f\n", (double)tv.tv_sec + (double)tv.tv_usec / 1000000);

        int ready = select(fd + 1, &descriptors, NULL, NULL, &tv);
        if (ready < 0){
            ERROR("select error");
        }
        if (ready == 0){
            break;
        }

        ssize_t bytes_read = recv(fd, buffer + total_bytes_read, BUFFER_SIZE - total_bytes_read, 0);
        if (bytes_read < 0)
            ERROR("recv error");
        if (bytes_read == 0){
            /* didnt read anything will close connection right aways*/
            return 0;
        }

        for (int i = 0; i < bytes_read-1; i++) {
            if (buffer[total_bytes_read + i] == '\n' && buffer[total_bytes_read + i + 1] == '\n'){
                buffer[total_bytes_read + i + 1] = '\0';
                break;
            }
        }

        //printf("DEBUG: %ld bytes read\n", bytes_read);
        total_bytes_read += bytes_read;

    }
    return 1;
}

/* write response to assoctiated fd*/
void respond(int fd, char *resp_buffer){
    /*might need to handle if cant send all at once*/
    if(send(fd, resp_buffer, strlen(resp_buffer), 0) == -1)
        perror("send error\n");
}