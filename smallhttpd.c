#include "sock.h"

int main(int ac, char *av[])
{
     int sock, fd;
     int *fdptr;
     pthread_t worker;
     pthread_attr_t attr;

     void *handle_call(void *);

     if (ac == 1) {
          fprintf(stderr, "usage; tws portnum\n");
          exit(1);
     }

     sock = make_server_socket(atoi(av[1]));
     if (sock == -1) {
          perror("making socket");
          exit(2);
     }

     setup(&attr);

     for (;;) {
          fd = accept(sock, NULL, NULL);
          server_requests++;

          fdptr = malloc(sizeof(int));
          *fdptr = fd;
          pthread_create(&worker, &attr, handle_call, fdptr);
          
     }
}
