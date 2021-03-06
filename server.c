#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>


int runCmd(char* cmd);
char** strsplit(char* str, char delim, size_t* numTokens);
int freeSplitStr(char** str);
void* runServer(void* fd);
void endServer();

volatile int runServerVar;
volatile int client_fd;
int sock_fd;
pthread_t cThread;

int main(int argc, char ** argv) {
    runServerVar =1;
    int s;
    sock_fd = socket(AF_INET, SOCK_STREAM,0);

    struct addrinfo hints, *results;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
  
    s = getaddrinfo(NULL, "1337", &hints, &results);
       
    if(s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	exit(1);
    }

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if(bind(sock_fd, results->ai_addr, results->ai_addrlen) != 0) {
      perror("bind failed");
      exit(1);
    }

    if(listen(sock_fd, 10) != 0) {
      perror("listen failed");
      exit(1);
    }

   struct sockaddr_in *result_addr = (struct sockaddr_in*) results->ai_addr;
   printf("Listening on fd %d, port %d\n", sock_fd, ntohs(result_addr->sin_port));
   
   printf("waiting...\n");
   int client_fd;
   while(runServerVar) {
	   client_fd = accept(sock_fd, NULL, NULL);
	   if(runServerVar) {
	       int retval = pthread_create(&cThread, NULL, runServer, (void*) client_fd);
	   } else { break;}
   }
   free(results);
   
}

void endServer() {
	fprintf(stderr, "ending server\n");
	runServerVar =0;
   	shutdown(sock_fd, SHUT_RDWR);
   	shutdown(client_fd, SHUT_RDWR);
   	close(client_fd);
        close(sock_fd);
   	write(8, "1337KODEhello4\n", 14);
	exit(1);
}


void* runServer(void* fd) {
   intptr_t client_fd = (intptr_t) fd;
   dup2(client_fd, STDOUT_FILENO);
   write(client_fd, "Connected\n", 11);
   char buffer[1000];
   int len;
   while(-1!= (len = read(client_fd, buffer, sizeof(buffer)-1)) ) {
      buffer[len] = '\0';
    int i = 0;
    int len = strlen(buffer);
    while(buffer[i]!='\0' && i<=len){
	if(buffer[i]=='\n'){
	    buffer[i]='\0';
        }
	i++;
    }
      if(0==strcmp(buffer, "exit")) { endServer(); return NULL;}
      
      printf("%s\n",buffer);
      runCmd(buffer);
   }

   
  return NULL;
}

int runCmd(char* cmd) {
  size_t numTokens;
   char** cmdArr = strsplit(cmd, ' ', &numTokens);

   if(!cmdArr) {return -1;}
   pid_t pid= fork();
   if(-1== pid) {printf("fork failed\n"); return -1;}
   else if(pid>0) {
     int status;
     free(cmdArr);
     waitpid(pid, &status, 0);
   } else {
      int exit_status = execvp(cmdArr[0], &cmdArr[0]);
      if( exit_status < 0) {printf("failed to exec %s: %s\n",cmdArr[0],strerror(errno));} 
	exit(0);
      
   }
   
}

char** strsplit(char* str, const char delim, size_t* numTokens) {
    char* savePtr;   
    char** splitStr = malloc(sizeof(char*)*2);
    char* tok=NULL;
    *numTokens=1;
    if(str[strlen(str)]=='\n') {str[strlen(str)]='\0';}
  
    splitStr[0] = strtok_r(str, &delim, &savePtr); 
    while( (splitStr[*numTokens] = strtok_r(NULL, &delim, &savePtr)) ) {
	(*numTokens)++;
	splitStr = realloc(splitStr, sizeof(char*) * (*numTokens));
    } 
 return splitStr; 
}

int freeSplitStr(char** str) {
  free(*str);
}
