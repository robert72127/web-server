#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(EXIT_FAILURE); }
#define BUFFER_SIZE 10000000    // 10 MB
#define MAXLINE 80
#define BACKLOG 16 // how many pending connections to hold
#define WAITTIME 10
/*stop parsing at double blank or error*/

// HTTP responses
#define OK 200 //succed
#define MOVED_PERMANENTLY 301 // want to download directory
#define FORBIDDEN 403 // want to download address laying beyond main directory
#define NOT_FOUND 404 // want to downolad non existing file
#define NOT_IMPLEMENTED 501 // browser sends incomprehensible data

struct request {
    char host[MAXLINE];
    char path[MAXLINE];
    char port[MAXLINE];
    float http_version;
    bool bad
};

// tcp communication.c
int open_listenfd(char *port);
ssize_t serve(int fd, int waittime, char **path);
void respond(int fd, char *resp_buffer);

// parse.c
int parse_request(char *buffer, struct request *req, char *path);
void create_response(struct request *req, char *resp_buffer,char *port,char *dir, bool bad);