#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

#include "webserver.h"

pid_t child;


void cleanup(int signal) {
    int status;
    while (waitpid((pid_t) (-1), 0, WNOHANG) > 0);
}

int main(int argc, char **argv) {
    signal(SIGCHLD, child_handler);
    
    char *buffer = malloc(sizeof(char) * BUFFER_SIZE); 
    struct response *resp = malloc(sizeof(struct response));  
    struct request *req = malloc(sizeof(struct request));

    int listenfd, confd;
    int wt;

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
        //child spawned process
        if (fork() == 0){ 
            close(listenfd);

            while(read_tcp(confd, wt, buffer)){
                parse_request(buffer, req, port);
                create_response(req, resp,directory);
                
                respond(confd,resp->header,resp->header_size);
                if (resp->message_size > 0) {
                    respond(confd,resp->message, resp->message_size);
                }
                
                memset(resp,0, sizeof(struct response));
                if(req->keep_conv){
                    wt = WAITTIME / 10 ;
                }
                else{
                    break;
                }

            }
            close(confd);
            free(req);
            free(buffer);
            free(resp);
            exit(0);
        }
        close(confd);

    }
    free(req);
    free(buffer);
    free(resp);
    close(listenfd);

}
