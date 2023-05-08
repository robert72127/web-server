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


int main(int argc, char **argv) {
    char *buffer = malloc(sizeof(char) * BUFFER_SIZE); 
    char *resp_buffer = malloc(sizeof(char) * BUFFER_SIZE); 
    struct request *req = malloc(sizeof(struct request));
    int listenfd, confd;
    int keep_conv = 0;


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
    while(1){
        
        /* file descriptor connected to client */
        confd = accept(listenfd, NULL, NULL);
        if(confd < 0){
            fprintf(stderr, "Connection error.\n");
            exit(0);
        } 
        
        printf("Accepted connection on port : %s\n", port);
        while(read_tcp(confd, WAITTIME, buffer)){
            keep_conv = parse_request(buffer, req, port);
            printf("KEEP_CONV: %d\n", keep_conv);
            if (keep_conv == 0){
                create_response(req, resp_buffer, port,directory, 1);
                respond(confd,resp_buffer);
                break;
            }
            else if (keep_conv == 2){
                create_response(req, resp_buffer,port,directory, 0);
                respond(confd,resp_buffer);
                break;
            }
            else if(keep_conv == 1){
                create_response(req, resp_buffer,port,directory, 0);
                respond(confd,resp_buffer);
            }
        }
        close(confd);


    }
    free(req);
    free(buffer);
    free(resp_buffer);
    close(listenfd);

}