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

/*path is correct if it doesn't reach beyond current dir and it exists*/
static bool check_path(char *path){
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
static void move(char **str){
    while(**str){
        if(**str == '\n'){
            *str++;
            break;
        }
        *str++;
    }
}
/*  parse and handle content of buffer
    return status and fills reuqest structure*/
int parse_request(char *buffer, struct request *req, char *path){
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
    float version;
    char *buff_ = buffer; 

    if (!sscanf(buff_,"GET %s HTTP/%f\n", path,version)){
        return 0;
    }
    // checking path
    memcpy(req->path, path, MAXLINE);
    req->http_version = version;
    move(&buff_);

    if(sscanf(buff_, "Host: %s:%s", host, port)){
        memcpy(req->port, port, MAXLINE);
    }
    else if(!sscanf(buff_,"Host: %s\n", host)){
        return 0;
    } 
    
    if(strcmp("localhost", host) && strcmp("virbian", host) && strcmp("virtual-domain.example.com", host)){
        return 0;
    }    
    memcpy(req->host, host, MAXLINE);
    move(&buff_);
    
    if(!sscanf(buff_,"Connection:close\n")){
        return 1;
    }
    return 2;
}

void create_response(struct request *req, char *resp_buffer, bool bad){
    if (bad){


    }
    // check port if there was any provided
    
    //check path
    if(! check_path(path)){
        return 0;
    }

    //write file

    /*clear req datastructure*/
    memset(req, 0, sizeof(struct request));

}

