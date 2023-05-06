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
#include <fcntl.h>
#include <string.h> 
#include <sys/stat.h>

#include "webserver.h"

const char *fextent(char *fname){
    char *ext = strrchr(fname, '.');
    if (!ext) {
        return "octet-stream";  
    }
    else if(!strcmp(ext+1, "html")){
        return "html";
    }
    else if(!strcmp(ext+1, "css")){
        return "css";
    }
    else if(!strcmp(ext+1, "jpg")){
        return "jpg";
    }
    else if(!strcmp(ext+1, "jpeg")){
        return "jpeg";
    }
    else if(!strcmp(ext+1, "png")){
        return "png";
    }
    else if(!strcmp(ext+1, "pdf")){
        return "pdf";
    }
    else {
        return "octet-stream";  
    }
}

int file_size(int fd){
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

//0 does not exists, 1 regular file, 2 directory
int check_file (char *filename) {
  struct stat  buffer;   
  if (stat (filename, &buffer) == 0){
    if( S_ISDIR(buffer.st_mode)){
        return 2;
    }
    return 1;
  }
  return 0;
}

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

static void move(char **str){
    *str = strrchr(*str, '\n') + 1;
}
/*  parse and handle content of buffer
    return status and fills reuqest structure*/
int parse_request(char *buffer, struct request *req, char *path){
    char host[MAXLINE];
    char port[MAXLINE];
    char *buff_ = buffer; 

    if (!sscanf(buff_,"GET %s HTTP/%f\n", req->path,req->http_version)){
        return 0;
    }
    // checking path
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

void create_response(struct request *req, char *resp_buffer, char *port,char *dir, bool bad){
    char *buf_ = resp_buffer;
    int file_status, fd, fsize;
    char full_path[MAXLINE];
    //treat bad port also as incomprehensible data
    if(strcmp(port, req->port)){
        bad = 1;
    }
    if (bad){
        sprintf(buf_, "HTTP/%f %d Not Implemented\n",req->http_version, NOT_IMPLEMENTED);
        move(&buf_);
        sprintf(buf_, "Content-Type: text/html\n");
        move(&buf_);
        sprintf(buf_, "Content-Length: %d text/html\n", 49);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);
        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>Bad request, not implemented!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n");
    
        memset(req, 0, sizeof(struct request));
        return;
    } 
    //check path
    if(! check_path(req->path)){
        sprintf(buf_, "HTTP/%f %d Forbidden\n",req->http_version, FORBIDDEN);
        move(&buf_);
        sprintf(buf_, "Content-Type: text/html\n");
        move(&buf_);
        sprintf(buf_, "Content-Length: %d text/html\n", 69);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);
        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>Trying to acces file laying beyond main directory!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n"); 
        
        memset(req, 0, sizeof(struct request));
        return;
    }
    file_status = check_file(req->path);
    if(file_status == 0){
        sprintf(buf_, "HTTP/%f %d Not Found\n",req->http_version, NOT_FOUND);
        move(&buf_);
        sprintf(buf_, "Content-Type: text/html\n");
        move(&buf_);
        sprintf(buf_, "Content-Length: %d text/html\n", 41);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);
        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>There is no such file!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n"); 
        
        memset(req, 0, sizeof(struct request));
        return;
    }
    else if(file_status == 1){
        sprintf(full_path,"%s/%s",dir, req->path);
        fd = open(full_path, O_RDONLY);
        fsize = file_size(fd); 
        
        sprintf(buf_, "HTTP/%f %d Ok\n",req->http_version, OK);
        move(&buf_);
        sprintf(buf_, "Content-Type: text/html\n");
        move(&buf_);
        sprintf(buf_, "Content-Length: %d text/html\n", fsize);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);
        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>Trying to acces file laying beyond main directory!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n"); 
        
        memset(req, 0, sizeof(struct request));
        return;
    }
    //directory
    else{
        sprintf(full_path,"%s/index.html",dir);
        fd = open(full_path, O_RDONLY);
        fsize = file_size(fd); 
        sprintf(buf_, "HTTP/%f %d Moved Permanently\n",req->http_version, MOVED_PERMANENTLY);
        move(&buf_);
        sprintf(buf_, "Content-Type: %s\n", fextent(req->path));
        move(&buf_);
        sprintf(buf_, "Content-Length: %d text/html\n", fsize);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);
        read(fd, buf_, fsize);
     
        memset(req, 0, sizeof(struct request));
        return;
    }
}