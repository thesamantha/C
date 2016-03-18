#include <stdio.h>
#include <sys/types.h>    //defines data types used in system calls, used types used in next two header files
#include <sys/socket.h>   //defines structures needed for sockets
#include <netinet/in.h>   //defines constants and structures needed for internet domain addresses

void error(char *msg);

int main(int argc, char *argv[]) {

  int sockfd, newsockfd, portno, clilen, n;

  /* sockfd and newsockfd are file descriptors (array subscripts in the file descriptor table). They store the values returned by the socket system call and accept system call, respectively. */

  /* portno stores the port number on which the server accepts connections */

  /* clilen stores the size of the client's address. This size is needed for the accept system call. */

  /* n is the return value for the read() and write() calls (i.e. it contains the number of characters read or written) */

  char buffer[256];   //characters from the socket connection are read into this buffer

  struct sockaddr_in serv_addr, cli_addr;

  /* sockaddr_in is a structure containing an internet address, defined in netinet/in.h as so:

    struct sockaddr_in {
  
      short sin_family;   //must be AF_INET
      u_short sin_port;
      struct  in_addr sin_addr;
      char sin_zero[8];   //not used, must be zero
    };

    An in_addr structure contains only one field, an unsigned long called s_addr.
  */

  if(argc < 2) {

    fprintf(stderr, "Error, no port provided.");
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);   //creates a new socket. 

  /* First argument is the address domain of the socket. AF_INET specifies the Address Family "Internet Protocol v4" of addresses. So this socket communicates only with addresses from this family. */

  /* Second argument is the type of socket. A stream socket (SOCK_STREAM) reads characters in a continuous stream (like from a file), and a datagram socket (SOCK_DGRAM) reads messages in chunks. */

  /* Third argument is the protocol. When set to 0, the operating system will choose the most appropriate protocol. Stream sockets get TCP, and datagram sockets get UDP. */

  /* The socket system call returns an entry into the file descriptor table (a small integer). This value is used for all subsequent references to this socket. Upon failure, the socket call returns -1. The program then displays an error message and exits. */

  if(sockfd < 0) {

    error("Error opening socket.");
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));    //sets all values in a buffer to zero. First argument is a pointer to the buffer and the second is the size of the buffer.

  portno = atoi(argv[1]);   //read port number from the command line
  serv_addr.sin_family = AF_INET;       //first field of sockaddr_in struct; code for the address family
  serv_addr.sin_port = htons(portno);   //second field of sockaddr_in; port number converted to network byte order using htons() 
  serv_addr.sin_addr.s_addr = INADDR_ANY;   //third field of sockadd_inr is an in_addr struct with single field unsigned long s_addr, containing the IP address of the host (for server, always the IP of the machine on which the server's running. INADDR_ANY gets this address.)

  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {

    error("Error on binding");
  }
  
  /* bind() binds a socket to an address. First argument is socket file descriptor, second is address to which socket should be bound (is a pointer to a struct of type sockaddr, so since we are passing a sockaddr_in we need to cast to sockaddr), third argument is size of the address). Can fail for a number of reasons, most obvious on being that the socket is already in use on this machine. */

  listen(sockfd, 5);    

  /* allows process to listen on the socket for connections. First argument is socket file descriptor, the second is the size of the backlog queue (how many connections can be waiting while process is handling another connection). Max size permitted by most systems is 5. Provided that sockfd is a valid socket, the call cannot fail. */

  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);   

  /* blocks process until a client connects to the server, returning a new file descriptor. All communication on established connection should be done using this file descriptor. Second argument is a reference pointer to the address of client on other end of the connection. */

  if(newsockfd < 0) {

    error("Error on accept.");
  }

  bzero(buffer, 256);     //initialize buffer
  n = read(newsockfd, buffer, 255);   //store number of characters read in n, using read with new socket file descriptor returned by accept. Note: read() blocks until the client has executed a write() (i.e. until there is something in the socket to be read)
  if(n < 0) {
  
    error("Error reading from socket.");
  }

  printf("Here is the message: %s", buffer);  //print client's message to console

  n = write(newsockfd, "I got your message", 18);   //write message to client, last argument being size of the message
  if(n < 0) {

    error("Error writing to socket.");
  }

  return 0;
}
 
