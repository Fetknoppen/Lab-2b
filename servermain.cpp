#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

/* You will to add includes here */
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"

#define MAXCLIENTS 20

using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount = 0;
int terminate = 0;

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum)
{
  // As anybody can call the handler, its good coding to check the signal number that called it.

  printf("Let me be, I want to sleep.\n");

  if (loopCount > 20)
  {
    printf("I had enough.\n");
    terminate = 1;
  }

  return;
}

int main(int argc, char *argv[])
{

  /* Do more magic */
  if (argc != 2)
  {
    printf("Invalid input.\n");
    exit(1);
  }
  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);
  if (Desthost == NULL || Destport == NULL)
  {
    printf("Invalid input.\n");
    exit(1);
  }

  int rv = 0;
  int yes = 1;
  int bytesRcv = 0;
  int bytesSent = 0;

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in clientAddr;

  struct hostent *hostp[MAXCLIENTS]; //client host info

  calcProtocol clcProt;
  calcMessage clcMsg;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  rv = getaddrinfo(NULL, Destport, &hints, &servinfo);
  if (rv != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  for (p = servinfo; p != NULL; p->ai_next)
  {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1)
    {
      printf("Litsener socket.\n");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
      printf("setsockopt faild.\n");
      close(sockfd);
      continue;
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      printf("listener bind.\n");
      close(sockfd);
      continue;
    }
    break;
  }
  if (p == NULL)
  {
    printf("Faild to bind socket.\n");
    exit(1);
  }

  freeaddrinfo(servinfo);

  /* 
     Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
  */
  struct itimerval alarmTime;
  alarmTime.it_interval.tv_sec = 10;
  alarmTime.it_interval.tv_usec = 10;
  alarmTime.it_value.tv_sec = 10;
  alarmTime.it_value.tv_usec = 10;

  /* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
  signal(SIGALRM, checkJobbList);
  setitimer(ITIMER_REAL, &alarmTime, NULL); // Start/register the alarm.

  while (terminate == 0)
  {
    memset(&clcMsg, 0, sizeof(clcMsg));
    memset(&clientAddr, 0, sizeof(clientAddr));

    bytesRcv = recvfrom(sockfd, &clcMsg, sizeof(clcMsg), NULL, p->ai_addr, &p->ai_addrlen);
    if (bytesRcv < 0)
    {
      printf("Could not recive.\n");
      exit(1);
    }
    if (clcMsg.type == 22 &&
        clcMsg.message == 0 &&
        clcMsg.protocol == 17 &&
        clcMsg.major_version == 1 &&
        clcMsg.minor_version == 0)
    {
      //OK
    }
    else{
      //wrong
    }

    printf("This is the main loop, %d time.\n", loopCount);
    sleep(1);
    loopCount++;
  }

  printf("done.\n");
  return (0);
}
