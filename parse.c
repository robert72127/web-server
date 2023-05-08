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

char *fextent(char *fname){
    char *ext = strrchr(fname, '.');
    if (!ext) {
        return "octet-stream";  
    }
    else if(!strcmp(ext+1, "html")){
        return "text/html";
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
    else { // don't know how to make it clean
        return "octet-stream";  
    }
}

//ok tested giver propper size in bytes
int file_size(int fd){
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

//ok
//0 does not exists, 1 regular file, 2 directory, 3 forbidden
int check_file (char *filename) {
  struct stat  buffer;   
  int size = strlen(filename);
  if (stat (filename, &buffer) == 0){
    if( S_ISDIR(buffer.st_mode)){
        for(int i = 0; i < size -2; i++){
            if (filename[i] == '.' && filename[i+1] == '.' && filename[i+2] == '/'){
                return 3;
            }
        }       
        return 2;
    }
    return 1;
  }
  return 0;
}


static void move(char **str){
    *str = strchr(*str, '\n') + 1;
}


/*  parse and handle content of buffer
    return status and fills reuqest structure*/
int parse_request(char *buffer, struct request *req, char *port){
    char *deli, *buff_ = buffer;
    if (sscanf(buff_,"GET %s HTTP/%f\n", req->path,&req->http_version) != 2){
        req->http_version = 2.0;
        return 0;
    }
    // checking path
    move(&buff_);



    if(sscanf(buff_,"Host: %s \n", req->host) == 1){
        deli = strchr(req->host, ':');
        if (deli){
            strcpy(req->port, deli+1);
            *deli = '\0';
            printf("REQ_PORT : %s\n REQ_HOST : %s\n", req->port, req->host);
        }
        else{
            printf("NUT FOUND :(\n");
            memcpy(req->port, port, MAXLINE);
        }
    }
    else{
        return 0;
    }

    if(strcmp("localhost", req->host) && strcmp("virbian", req->host) && strcmp("virtual-domain.example.com", req->host)){
        return 0;
    }    
    move(&buff_);
    
    if(!*buff_){
        return 1;
    }
    else if(!strcmp(buff_,"Connection: close\r\n")){
        return 2;
    }
    
    return 0;
}

//GET / HTTP/1.1
//Host: localhost
//Connection: close

static char *create_header(char *buf_, char *message, char *content_type, struct request *req,int len, int code){
        sprintf(buf_, "HTTP/%.2f %d %s\n",req->http_version, code, message);
        move(&buf_);
        sprintf(buf_, "Content-Type: %s\n", content_type);
        move(&buf_);
        sprintf(buf_, "Content-Length: %d text/html\n", len);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);

        return buf_;
}

void create_response(struct request *req, char *resp_buffer, char *port,char *dir, bool bad){
    char *buf_ = resp_buffer;
    int file_status, fd, fsize;
    char full_path[MAXLINE];
    //treat bad port also as incomprehensible data
    if(!req->port ||  strcmp(port, req->port)){
        bad = 1;
    }
    if (bad){
        buf_ = create_header(buf_, "Not Implemented", "text/html",req,49,NOT_IMPLEMENTED);
        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>Bad request, not implemented!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n");
    
        memset(req, 0, sizeof(struct request));
        return;
    } 
    //check path
    sprintf(full_path,"%s%s",dir, req->path);
    file_status = check_file(full_path);

    switch (file_status)
    {
    case 0:
        buf_ = create_header(buf_, "Not Found", "text/html",req,41,NOT_FOUND);
        
        move(&buf_);
        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>There is no such file!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n"); 
        break;
    case 1:
        sprintf(full_path,"%s%s",dir, req->path);
        fd = open(full_path, O_RDONLY);
        fsize = file_size(fd); 
        
        buf_ = create_header(buf_, "OK", fextent(req->path),req,fsize,OK);

        read(fd, buf_, fsize);

        break;
    case 2:
        sprintf(full_path,"%s/virbian/index.html",dir);
        fd = open(full_path, O_RDONLY);
        fsize = file_size(fd); 

        buf_ = create_header(buf_, "Moved Permanently", "text/html",req,fsize,MOVED_PERMANENTLY);
        
        read(fd, buf_, fsize);
        break;
    case 3:
        buf_ = create_header(buf_, "Forbidden", "text/html",req,69,FORBIDDEN);

        sprintf(buf_, "<html>\n");
        move(&buf_);
        sprintf(buf_, "<p>Trying to acces file laying beyond main directory!</p>\n");
        move(&buf_);
        sprintf(buf_, "</html>\n"); 
        break;
    }
    memset(req, 0, sizeof(struct request));
}
