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

//0 does not exists, 1 regular file, 2 forbidden, 3 directory
int check_file (char *filename) {
  struct stat  buffer;   
  int size = strlen(filename);
  if (stat (filename, &buffer) == 0){
    if( S_ISDIR(buffer.st_mode)){
        for(int i = 0; i < size -2; i++){
            if (filename[i] == '.' && filename[i+1] == '.' && filename[i+2] == '/'){
                return 2;
            }
        }       
        return 3;
    }
    return 1;
  }
  return 0;
}


static void move(char **str){
    *str = strchr(*str, '\n') + 1;
}

/* fills req structure with content from buffer */
void parse_request(char *buffer, struct request *req, char *port){ 
    req->bad = 0;
    req->keep_conv = 1;
    char path_[MAXLINE];

    char *hlp,*deli, *buff_ = buffer;

    if (sscanf(buff_,"GET %s HTTP/%f\n", path_,&req->http_version) != 2){
        req->http_version = 2.0;
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

        //sprintf(req->path,"/%s", req->host);
        sprintf(req->path,"%s", req->host);
        hlp = strchr(req->path, '\0');
        if (hlp)
            sprintf(hlp,"%s", path_);
    }
    else{
        req->bad = 1;
        return;
    }

    if(strcmp("localhost", req->host) && strcmp("virbian", req->host) && strcmp("virtual-domain.example.com", req->host)){
        req->bad = 1;
        return;
    }    
    move(&buff_);
    

    if(buff_ && strstr(buff_,"Connection: close")){
        req->keep_conv = 0;
    }
}

//header is written to buf_, and return value is it's size
static int create_header(char *buf_, char *message, char *content_type, struct request *req,int len, int code){
        char *start = buf_;
        sprintf(buf_, "HTTP/%.2f %d %s\n",req->http_version, code, message);
        move(&buf_);
        
        if(code == MOVED_PERMANENTLY){
            sprintf(buf_, "Location: http://%s:%s/index.html\n", req->host, req->port);
            move(&buf_);
            sprintf(buf_, "\n");
            move(&buf_);
        }
        else{
            sprintf(buf_, "Content-Type: %s\n", content_type);
            move(&buf_);
            sprintf(buf_, "Content-Length: %d\n", len);
            move(&buf_);
            sprintf(buf_, "\n");
            move(&buf_);
        }

        return (int)(buf_ - start)/ (sizeof (char)) ;  
}

// fills response structure
void create_response(struct request*req, struct response *resp,char *dir){
    char *buf_ = resp->header;
    int file_status;
    FILE *fd;
    char full_path[MAXLINE*2+2];
    //treat bad port also as incomprehensible data
    if (req->bad){
        resp->message_size =  51;
        resp->header_size = create_header(buf_, "Not Implemented", "text/html",req,resp->message_size,NOT_IMPLEMENTED);
        sprintf(resp->message, "<html>\n<p>Bad request, not implemented!</p>\n</html>\n");
    
        memset(req, 0, sizeof(struct request));
        return;
    }

    //check path
    sprintf(full_path,"%s%s",dir, req->path);
    file_status = check_file(full_path);
    int kind;
    char *extension = fextent(req->path, &kind); 

    switch (file_status)
    {
    case 0:
        resp->message_size = 45;
        resp->header_size = create_header(buf_, "Not Found", extension,req,resp->message_size,NOT_FOUND);
        sprintf(resp->message, "<html>\n<p>There is no such file!</p>\n</html>\n");
        break;
    case 1:
        sprintf(full_path,"%s%s",dir, req->path);
        if(!kind)
            fd = fopen(full_path, "rb");
        else
            fd = fopen(full_path, "r");

        resp->message_size = file_size(fd);         
        
        create_header(buf_, "OK", extension,req,resp->message_size,OK);

        fread(resp->message, resp->message_size,1, fd);
        break;
    case 2:
        resp->message_size = 69;
        resp->header_size =  create_header(buf_, "Forbidden", extension,req,resp->message_size,FORBIDDEN);
        sprintf(resp->message, "<html>\n<p>Trying to acces file laying beyond main directory!</p>\n</html>\n"); 
        break;
    case 3:
        resp->message_size = 0;
        resp->header_size =  create_header(buf_, "Moved Permanently", extension,req,resp->message_size, MOVED_PERMANENTLY);
        break;
    }
    memset(req, 0, sizeof(struct request));

}
