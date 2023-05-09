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
    char *header_buffer = malloc(sizeof(char) * BUFFER_SIZE); 
    struct request *req = malloc(sizeof(struct request));
    int listenfd, confd;
    int keep_conv = 0;
    int wt;
    int fsize;

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
       wt = WAITTIME; 
        /* file descriptor connected to client */
        confd = accept(listenfd, NULL, NULL);
        if(confd < 0){
            fprintf(stderr, "Connection error.\n");
            exit(0);
        } 
        
        //printf("Accepted connection on port : %s\n", port);
        while(read_tcp(confd, wt, buffer)){
            keep_conv = parse_request(buffer, req, port);
            if (keep_conv == 0){
               fsize =  create_response(req,header_buffer, resp_buffer, port,directory, 1);
                //send header
                respond(confd,header_buffer,0);
                respond(confd,resp_buffer, fsize);
                //send data
                break;
            }
            else if (keep_conv == 2){
                fsize = create_response(req,header_buffer, resp_buffer,port,directory, 0);
                printf("%s\n", resp_buffer);
                respond(confd,header_buffer,0);
                respond(confd,resp_buffer, fsize);
                break;
            }
            else if(keep_conv == 1){
                fsize = create_response(req,header_buffer, resp_buffer,port,directory, 0);
                printf("%s\n", resp_buffer);
                respond(confd,header_buffer,0);
                respond(confd,resp_buffer, fsize);
                wt = WAITTIME/10;
            }
                
            fsize = create_response(req,header_buffer, resp_buffer,port,directory, keep_conv == 0);
            respond(confd,header_buffer,0);    
            respond(confd,resp_buffer, fsize);
            if(keep_conv == 0 || keep_conv == 1){
                break;
            }
            else{
                wt = WAITTIME/10;
            }

        }
        close(confd);


    }
    free(req);
    free(buffer);
    free(header_buffer);
    free(resp_buffer);
    close(listenfd);

}