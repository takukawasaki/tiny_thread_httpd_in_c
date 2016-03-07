#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/socket.h>
#include <time.h>
#include <strings.h>
#include <netdb.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>


#define PORTNUM 8080
#define HOSTLEN 256
#define BACKLOG  1
#define oops(m) {perror(m); exit(1);}


time_t server_started;
int server_bytes_sent;
int server_requests;


int make_server_socket(int );
int make_server_socket_q(int, int);
int connect_to_server(char *, int);

void process_request(int);
void talk_with_server(int);
void read_til_crnl(FILE *);
void process_rq(char *, int fd);
void header(FILE *fp, char *);
void cannot_do(int );
int do_404(char *, int );
int isadir(char *);
int not_exist(char *);
int do_ls(char *, int);
char * file_type(char *);
int ends_in_cgi(char *);
void do_exec(char *, int);
int do_cat(char *, int);
void *handle_call(void *fdptr);
void setup(pthread_attr_t *);
void skip_rest_of_header(FILE *);
void sanitize(char *);
int built_in(char *, int);
int http_reply(int, FILE **, int, char *, char *, char *);
int not_implemented(int );

