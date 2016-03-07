#include "sock.h"

int make_server_socket(int portnum)
{
     return make_server_socket_q(portnum,BACKLOG);
}

int make_server_socket_q(int portnum, int backlog)
{
     struct sockaddr_in saddr, cliaddr;
     struct hostent *hp;
     char hostname[HOSTLEN];
     int sock_id;

     sock_id = socket(AF_INET, SOCK_STREAM, 0);
     if (sock_id == -1) {
          return -1;
     }

     bzero((void *)&saddr, sizeof(saddr));
     gethostname(hostname, HOSTLEN);
     hp = gethostbyname(hostname);
     saddr.sin_addr.s_addr = htonl(INADDR_ANY);
     saddr.sin_port = htons(portnum);
     saddr.sin_family = AF_INET;

     if (bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
          return -1;
     }

     if (listen(sock_id, backlog) != 0) {
          return -1;
          
     }
     return sock_id;
}


int connect_to_server(char *host, int portnum)
{
     int sock;
     struct sockaddr_in saddr;
     struct hostent *hp;

     sock = socket(AF_INET, SOCK_STREAM, 0);
     if (sock == -1) {
          return -1;
          
     }
     bzero(&saddr, sizeof(saddr));
     hp = gethostbyname(host);
     if (hp == NULL) {
          return -1;
     }

     bcopy(hp->h_addr, (struct sockaddr *)&saddr.sin_addr, hp->h_length );
     saddr.sin_port = htons(portnum);
     saddr.sin_family = AF_INET;

     if (connect (sock, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
          return -1;
     }

     return sock;
     
}

void process_request(int fd)
{
     int pid = fork();
     switch (pid) {
     case -1: {
          return;
          break;
     }
     case 0: {
          dup2(fd,1);
          close(fd);
          execl("/bin/date","date",NULL);
          oops("execlp");
          break;
     }
     default:
          wait(NULL);
          break;
     }
}

void talk_with_server(int fd)
{
     char buf[BUFSIZ];
     int n;
     n = read(fd,buf,BUFSIZ);
     write(1,buf,n);
     
}
void
read_til_crnl(FILE *fp)
{
     char buf[BUFSIZ];
     while (fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
          ;
     }
     
}

void
process_rq( char *rq, int fd )
{
     char cmd[BUFSIZ], arg[BUFSIZ];

     if (sscanf(rq, "%s%s", cmd, arg) != 2) {
          return;
     }

     sanitize(arg);
     printf("saitized version is %s\n", arg);
     

     if ( strcmp(cmd,"GET") != 0 )
          not_implemented(fd);

     else if (built_in(arg, fd))
          ;
     else if ( not_exist( arg ) )
          do_404(arg, fd );
     else if ( isadir( arg ) )
          do_ls( arg, fd );

     else
          do_cat( arg, fd );
     
}

void sanitize(char *str)
{
     char *src, *dest;

     src = dest = str;

     while (*src) {
          if (strncmp(src, "/../",4) == 0) {
               src += 3;
          }else if(strncmp(src,"//",2) == 0){
               src++;
          }else{
               *dest++ = *src++;
               
          }
          *dest = '\0';
          if (*str == '/') {
               strcpy(str, str+1);
          }
          if (str[0] == '\0' || strcmp(str, "./") == 0 || strcmp(str,"./..") == 0) {
               strcpy(str,".");
          }
          
     }
}

int built_in(char *arg, int fd)
{
     FILE *fp;

     if (strcmp(arg,"status") != 0) {
          return 0;
     }
     http_reply(fd, &fp, 200, "OK", "text/plain",NULL);

     fprintf(fp,"server started: %s",ctime(&server_started));
     fprintf(fp,"Total requests: %d\n", server_requests);
     fprintf(fp,"bytes sent out: %d\n", server_bytes_sent);
     fclose(fp);
     return 1;
     
}


int http_reply(int fd, FILE **fpp, int code, char *msg, char *type, char *content)
{
     FILE *fp = fdopen(fd, "w");
     int bytes = 0;

     if (fp != NULL) {
          bytes = fprintf(fp, "HTTP/1.0 %d %s\r\n", code,msg);
          bytes += fprintf(fp, "Content-type: %s\r\n\r\n", type);
          if (content) {
               bytes += fprintf(fp, "%s\r\n", content);
          }
          
     }
     fflush(fp);
     if (fpp) {
          *fpp =fp;
     }else {
          fclose(fp);
     }
     return bytes;
}

int not_implemented(int fd)
{
     
     return http_reply(fd,NULL, 501, "Not Implemented", "text/plain",
                "That command is not implemented");
     
}


/* ------------------------------------------------------ *
   the reply header thing: all functions need one
   if content_type is NULL then don't send content type
   ------------------------------------------------------ */
void
header( FILE *fp, char *content_type )
{
     fprintf(fp, "HTTP/1.0 200 OK\r\n");
     if ( content_type )
          fprintf(fp, "Content-type: %s\r\n", content_type );
     
}


/* ------------------------------------------------------ *
   simple functions first:
        cannot_do(fd)       unimplemented HTTP command
    and do_404(item,fd)     no such object
    ------------------------------------------------------ */

void
cannot_do(int fd)
{
     FILE *fp = fdopen(fd,"w");

     fprintf(fp, "HTTP/1.0 501 Not Implemented\r\n");
     fprintf(fp, "Content-type: text/plain\r\n");
     fprintf(fp, "\r\n");

     fprintf(fp, "That command is not yet implemented\r\n");
     
}

int 
do_404(char *item, int fd)
{
     
     return http_reply(fd, NULL, 404, "Not Found", "text/plain",
                "The item you seed is not here");
}


/* ------------------------------------------------------ *
   the directory listing section
   isadir() uses stat, not_exist() uses stat
   do_ls runs ls. It should not
   ------------------------------------------------------ */
int isadir(char *f)
{
     
     struct stat info;
     return ( stat(f, &info) != -1 && S_ISDIR(info.st_mode) );
}

int
not_exist(char *f)
{
     struct stat info;
     return( stat(f,&info) == -1 );
     
}

int
do_ls(char *dir, int fd)
{
     
     DIR      *dirptr;
     struct dirent *direntp;
     FILE      *fp;
     int      bytes = 0;

     bytes = http_reply(fd,&fp,200,"OK","text/plain",NULL);
     bytes += fprintf(fp,"Listing of Directory %s\n", dir);

     if ( (dirptr = opendir(dir)) != NULL ){
          while( (direntp = readdir(dirptr)) ){
               bytes += fprintf(fp, "%s\n", direntp->d_name);
               
          }
          closedir(dirptr);
          
     }
     fclose(fp);
     return server_bytes_sent += bytes;
     
}



/* ------------------------------------------------------ *
   the cgi stuff.  function to check extension and
   one to run the program.
   ------------------------------------------------------ */

char * file_type(char *f)
/* returns 'extension' of file */
{
     char*cp;
     if ( (cp = strrchr(f, '.' )) != NULL )
          return cp+1;
     return "";
     
}

int
ends_in_cgi(char *f)
{
     return ( strcmp( file_type(f), "cgi" ) == 0 );
     
}

void
do_exec( char *prog, int fd )
{
     FILE*fp ;

     fp = fdopen(fd,"w");
     header(fp, NULL);
     fflush(fp);
     dup2(fd, 1);
     dup2(fd, 2);
     close(fd);
     execl(prog,prog,NULL);
     perror(prog);
}

/* ------------------------------------------------------ *
   do_cat(filename,fd)
   sends back contents after a header
   ------------------------------------------------------ */

int 
do_cat(char *f, int fd)
{
     char *extension = file_type(f);
     char *type = "text/plain";
     FILE *fpsock, *fpfile;
     int c;
     int bytes = 0;

     if ( strcmp(extension,"html") == 0 )
          type = "text/html";
     else if ( strcmp(extension, "gif") == 0 )
          type = "image/gif";
     else if ( strcmp(extension, "jpg") == 0 )
          type = "image/jpeg";
     else if ( strcmp(extension, "jpeg") == 0 )
          type = "image/jpeg";

     fpsock = fdopen(fd, "w");
     fpfile = fopen( f , "r");
     if ( fpsock != NULL && fpfile != NULL )
     {
          bytes = http_reply(fd, &fpsock, 200, "OK", type, NULL);
          while ((c = getc(fpfile)) != EOF) {
               putc(c, fpsock);
               bytes++;
          }
          fclose(fpfile);
          fclose(fpsock);
          
     }
     
    return  server_bytes_sent += bytes;
}



void setup(pthread_attr_t *attrp)
{
     pthread_attr_init(attrp);
     pthread_attr_setdetachstate(attrp, PTHREAD_CREATE_DETACHED);
     time(&server_started);
     server_requests=0;
     server_bytes_sent = 0;
     
}

void *handle_call(void *fdptr)
{
     
     FILE *fpin;
     char request[BUFSIZ];
     int fd;

     
     fd = *(int *)fdptr;
     free(fdptr);

     fpin = fdopen(fd, "r");
     fgets(request, BUFSIZ, fpin);
     printf("got a call on %d: request = %s",fd, request);
     skip_rest_of_header(fpin);

     process_rq(request, fd);

     fclose(fpin);

}


void skip_rest_of_header(FILE *fp)
{
     char buf[BUFSIZ];
     while (fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
          ;
     }
}


