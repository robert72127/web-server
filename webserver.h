#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(EXIT_FAILURE); }
#define BUFFER_SIZE 10000000    // 10 MB
#define MAXLINE 80
#define BACKLOG 16 // how many pending connections to hold
#define WAITTIME 10
/*stop parsing at double blank or error*/
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
void create_response(struct request *req, char *resp_buffer, bool bad);