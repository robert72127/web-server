#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "webserver.h"

//kind: 0 is binary, 1 is utf
char *fextent(char *fname, int *kind){
    char *ext = strrchr(fname, '.');
    if (!ext) {
        *kind = 0;
        return "octet-stream";  
    }
    else if(!strcmp(ext+1, "txt")){
        *kind = 1;
        return "text/plain; charset=utf-8";
    }
    else if(!strcmp(ext+1, "html")){
        *kind = 1;
        return "text/html; charset=utf-8";
    }
    else if(!strcmp(ext+1, "css")){
        *kind = 1;
        return "text/css; charset=utf-8";
    }
    else if(!strcmp(ext+1, "jpg")){
        *kind = 0;
        return "image/jpeg; charset=binary";
    }
    else if(!strcmp(ext+1, "jpeg")){
        *kind = 0;
        return "image/jpeg; charset=binary";
    }
    else if(!strcmp(ext+1, "png")){
        *kind = 0;
        return "image/png; charset=binary";
    }
    else if(!strcmp(ext+1, "pdf")){
        *kind = 0;
        return "application/pdf; charset=binary";
    }
    else { // don't know how to make it clean
        *kind = 0;
        return "octet-stream; charset=binary";  
    }
}

int file_size(FILE *fp){
    int sz;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return sz;
}

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
static void move_last(char **str){
    *str = strrchr(*str, '\n') + 1;
}

/*  parse and handle content of buffer
    return status and fills reuqest structure*/
int parse_request(char *buffer, struct request *req, char *port){ 
    char path_[MAXLINE];
    char *hlp,*deli, *buff_ = buffer;

    if (sscanf(buff_,"GET %s HTTP/%f\n", path_,&req->http_version) != 2){
        req->http_version = 2.0;
        return 0;
    }
    move(&buff_);
    
    if(sscanf(buff_,"Host: %s \n", req->host) == 1){
        deli = strchr(req->host, ':');
        if (deli){
            strcpy(req->port, deli+1);
            *deli = '\0';
        }
        else{
            memcpy(req->port, port, MAXLINE);
        }

        sprintf(req->path,"/%s", req->host);
        hlp = strchr(req->path, '\0');
        sprintf(hlp,"%s", path_);

    }
    else{
        return 0;
    }

    if(strcmp("localhost", req->host) && strcmp("virbian", req->host) && strcmp("virtual-domain.example.com", req->host)){
        return 0;
    }    
    move_last(&buff_);
    
    if(buff_ && sscanf(buff_,"Connection: close\r\n")){
        return 2;
    }
    
    return 1;
}


static void create_header(char *buf_, char *message, char *content_type, struct request *req,int len, int code){
        sprintf(buf_, "HTTP/%.2f %d %s\n",req->http_version, code, message);
        move(&buf_);
        sprintf(buf_, "Content-Type: %s\n", content_type);
        move(&buf_);
        sprintf(buf_, "Content-Length: %d\n", len);
        move(&buf_);
        sprintf(buf_, "\n");
        move(&buf_);

}

int create_response(struct request *req,char *header_buffer, char *resp_buffer, char *port,char *dir, bool bad){
    char *buf_ = header_buffer;
    int file_status, fsize;
    FILE *fd;
    char full_path[MAXLINE*2];
    //treat bad port also as incomprehensible data
    if(!req->port ||  strcmp(port, req->port)){
        bad = 1;
    }
    if (bad){
        create_header(buf_, "Not Implemented", "text/html",req,49,NOT_IMPLEMENTED);
        sprintf(resp_buffer, "<html>\n<p>Bad request, not implemented!</p>\n</html>\n");
    
        memset(req, 0, sizeof(struct request));
        fsize = 49;
        return fsize;
    } 
    //check path
    sprintf(full_path,"%s%s",dir, req->path);
    file_status = check_file(full_path);
    int kind;
    char *extension = fextent(req->path, &kind); 


    switch (file_status)
    {
    case 0:
        fsize = 41;
        create_header(buf_, "Not Found", extension,req,41,NOT_FOUND);
        sprintf(resp_buffer, "<html>\n<p>There is no such file!</p>\n/html>\n");
        break;
    case 1:
        sprintf(full_path,"%s%s",dir, req->path);
        if(!kind)
            fd = fopen(full_path, "rb");
        else
            fd = fopen(full_path, "r");

        fsize = file_size(fd);         
        
        create_header(buf_, "OK", extension,req,fsize,OK);

        fread(resp_buffer, fsize,1, fd);
        break;
    case 2:
        sprintf(full_path,"%s/virbian/index.html",dir);
        fd = fopen(full_path, "r");
        fsize = file_size(fd); 

        create_header(buf_, "Moved Permanently", extension,req,fsize,MOVED_PERMANENTLY);
        fread(resp_buffer,fsize,1,fd);
        break;
    case 3:
        fsize = 69;
        create_header(buf_, "Forbidden", extension,req,69,FORBIDDEN);
        sprintf(resp_buffer, "<html>\n<p>Trying to acces file laying beyond main directory!</p>\n</html>\n"); 
        break;
    }
    memset(req, 0, sizeof(struct request));
    return fsize;
}
