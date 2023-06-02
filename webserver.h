#pragma once

#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(EXIT_FAILURE); }
#define BUFFER_SIZE 10000000    // 10 MB
#define MAXLINE 80
#define BACKLOG 16 // how many pending connections to hold
#define WAITTIME 10
#define MAXCHUNK 10000
/*stop parsing at double blank or error*/

// HTTP responses
#define OK 200 //succed
#define MOVED_PERMANENTLY 301 // want to download directory
#define FORBIDDEN 403 // want to download address laying beyond main directory
#define NOT_FOUND 404 // want to downolad non existing file
#define NOT_IMPLEMENTED 501 // browser sends incomprehensible data

struct request {
    char path[2*MAXLINE];
    char port[MAXLINE];
    char host[MAXLINE];
    float http_version;
    bool keep_conv;
    bool bad;
};
struct response{
    char header[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int header_size;
    int message_size;
};


// tcp communication.c
int open_listenfd(char *port);
ssize_t read_tcp(int fd, int waittime, char *buffer);
void respond(int fd, char *resp_buffer, int size);

// parse.c
void parse_request(char *buffer, struct request *req, char *port);
void create_response(struct request *req,struct response *resp,char *dir);
