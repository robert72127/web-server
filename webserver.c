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

static char buffer[BUFFER_SIZE]; 

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


ssize_t serve (int fd, int waittime, char **path){
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
            if (buffer[total_bytes_read + i] == '\n' && buffer[total_bytes_read + i + 1] == '\n')
                buffer[total_bytes_read + i + 1] = '\0';
                break;
        }

        //printf("DEBUG: %ld bytes read\n", bytes_read);
        total_bytes_read += bytes_read;

    }
    return parse_request(path);
    /*

    int end = (total_bytes_read < BUFFER_SIZE )? total_bytes_read: BUFFER_SIZE-1;
    buffer[end] = '\0';
    printf("%s\n",buffer);
    // ok easy sending back 
    if (send(fd, "Hello, fucker!", 15, 0) == -1)
        perror("send");

    return 0;
    */
}
/*path is correct if it doesn't reach beyond current dir and it exists*/
int check_path(char *path){
    int size = strlen(path);
    /*could check recursively but assuming inclusion of ../ is malicious is better heursitsics*/
    for(int i = 0; i < size -2; i++){
        if (path[i] == '.' && path[i+1] == '.' && path[i+2] == '/')
            return 0;
    }
    /* check if directory exists */
    DIR* dir = opendir(path);
    if(dir){
        /* it exists*/
        closedir(dir);
        return 1;
    }
    else{
        return 0;
    }
}
/*need to think bit more on that*/
char move(char **str){
    while(*str){
        if(*str == '\n'){
            *str++;
            *str = str;
        }
        *str++;
    }
}

/* parse and handle content of buffer*/
int parse_request(char **path){
    char host[MAXLINE];
    
    char *buff_ = buffer; 

    if (!sscanf(buff_,"GET %s HTTP/%f\n",*path)){
        return 0;
    }
    // checking path
    if(! check_path(*path)){
        return 0;
    }
    move(&buff_);
    if(!sscanf(buff_,"Host: %s\n", host)){
        return 0;
    } 
    if(strcmp("localhost", host) && strcmp("virbian", host) && strcmp("virtual-domain.example.com", host)){
        return 0;
    }    
    move(&buff_);
    if(!sscanf(buff_,"Connection:close\n")){
        return 1;
    }
    return 2;
}

/* write response to assoctiated fd*/
void respond(char *path){
    sprintf("")


}

int main(int argc, char **argv) {
    int listenfd, confd;
    char path[MAXLINE];

    struct sockaddr_storage clientaddr;

    socklen_t clientlen;

    if(argc != 3){
        fprintf(stderr, "Usage: webserver <port> <directory>.\n");
        exit(0);
    }
    char *port = argv[1];
    char *directory = argv[2];
    
    /* check if directory exists */
    DIR* dir = opendir(directory);
    if(dir){
        /* it exists*/
        closedir(dir);
    }
    else{
        fprintf(stderr, "Error directory does not exists.\n");
        exit(0);
    }

    /* create listening descriptor */
    listenfd = open_listenfd(port);
    /* main loop */
    bool keep_conv;
    while(1){
        
        /* file descriptor connected to client */
        confd = accept(listenfd, NULL, NULL);
        if(confd < 0){
            fprintf(stderr, "Connection error.\n");
            exit(0);
        } 
        
        printf("Accepted connection on port : %s\n", port);

        keep_conv = serve(confd, WAITTIME, &path);
        /* connection close was send */
        if(keep_conv == 2){
            respond(path);
            continue;
        } 
        /* wait for further requests */
        while(keep_conv){
            respond(path);

            // and then wait for maybe more messages
            serve(confd, WAITTIME, &path);
        }
        
        close(confd);

    }
    close(listenfd);

}