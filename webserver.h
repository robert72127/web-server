#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(EXIT_FAILURE); }
#define BUFFER_SIZE 10000000    // 10 MB
#define MAXLINE 80
#define BACKLOG 16 // how many pending connections to hold
#define TIMEOUT 10 // timeout of 10 seconds :O

/*stop parsing at double blank or error*/
struct request {
    char resource[MAXLINE];
    char host[MAXLINE];
    char connection[MAXLINE];
};