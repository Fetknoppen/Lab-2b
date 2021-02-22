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
#include <string>
#include <string.h>
#include <iostream>
#include <chrono>
#include <ctime> 

// Included to get the support library
#include <calcLib.h>

#include "protocol.h"

#define MAXCLIENTS 20
#define MAXLINE 1024 //Change

using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount = 0;
bool run = true;
int id = 1;
struct clientInfo
{
  int id;
  int arith;
  int inValue1, inValue2, inResult;
  float flValue1, flValue2, flResult;
  char clientAddress[250]; //ip:port
  time_t start;
  struct clientInfo *next;
};
clientInfo currentClient;

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum)
{
  // As anybody can call the handler, its good coding to check the signal number that called it.

  time_t end = time(0);
  clientInfo *current = &currentClient;
  clientInfo ref = currentClient;
  int nrOfRemoved = 0;

  while (current->next != NULL)
  {
    if(end - current->start >= 10){
      printf("Removed client: %d.\n", current->id);
      memset(current, 0, sizeof(current));
      current = nullptr;
      nrOfRemoved++;
    }
    current = current->next;
  }
  currentClient = ref;
  if(nrOfRemoved == 0){
    printf("No removed clients.\n");
  }

  return;
}
in_port_t get_in_port(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
    return (((struct sockaddr_in *)sa)->sin_port);

  return (((struct sockaddr_in6 *)sa)->sin6_port);
}
int getArith()
{

  string randArith = randomType();
  if (randArith == "add")
    return 1;
  else if (randArith == "sub")
    return 2;
  else if (randArith == "mul")
    return 3;
  else if (randArith == "div")
    return 4;
  else if (randArith == "fadd")
    return 5;
  else if (randArith == "fsub")
    return 6;
  else if (randArith == "fmul")
    return 7;
  else if (randArith == "fdiv")
    return 8;
  else
  {
    return -1;
  }
}

void addClient(clientInfo *client, calcProtocol &clcProt, struct sockaddr_in &clientAddress)
{
  clientInfo *current = client;
  while (current->next != NULL)
  {
    current = current->next;
  }

  currentClient = *current;

  
  current->next = (clientInfo *)malloc(sizeof(clientInfo));
  sprintf(current->next->clientAddress, "%s:%d", inet_ntoa(clientAddress.sin_addr), htons(clientAddress.sin_port));

  current->start = time(0);
  current->next->id = clcProt.id;
  current->next->arith = clcProt.arith;
  current->next->inValue1 = clcProt.inValue1;
  current->next->inValue2 = clcProt.inValue2;
  current->next->flValue1 = clcProt.flValue1;
  current->next->flValue2 = clcProt.flValue2;

  //Calculate andsave the result
  switch (ntohl(current->next->arith))
  {
  case 1:
    current->next->inResult = ntohl(current->next->inValue1) + ntohl(current->next->inValue2);
    break;
  case 2:
    current->next->inResult = ntohl(current->next->inValue1) - ntohl(current->next->inValue2);
    break;
  case 3:
    current->next->inResult = ntohl(current->next->inValue1) * ntohl(current->next->inValue2);
    break;
  case 4:
    current->next->inResult = ntohl(current->next->inValue1) / ntohl(current->next->inValue2);
    break;
  case 5:
    current->next->flResult = current->next->flValue1 + current->next->flValue2;
    break;
  case 6:
    current->next->flResult = current->next->flValue1 - current->next->flValue2;
    break;
  case 7:
    current->next->flResult = current->next->flValue1 * current->next->flValue2;
    break;
  case 8:
    current->next->flResult = current->next->flValue1 / current->next->flValue2;
    break;

  default:
    break;
  }
  //Convert to network byte order
  if (ntohl(current->next->arith) < 5)
  {
    current->next->inResult = htonl(current->next->inResult);
  }

  current->next->next = NULL;
}
void checkJob(clientInfo *client, char *Iaddress, calcProtocol *clcProt, calcMessage &clcMsg)
{
  clientInfo *current = client;
  bool searching = true;
  bool found = false;

  while (searching)
  {
    if (current->id == clcProt->id)
    {
      searching = false;
      found = true;
    }
    else if (current->next == NULL)
    {
      searching = false;
    }
    else
    {
      current = current->next;
    }
  }
  if (found)
  {
    //Ceck if it is the right client
    if (strcmp(current->clientAddress, Iaddress) == 0)
    {
      if (ntohl(current->arith) < 5)
      {
        printf("int result.\nServer awnser: %d.\nClient awnser: %d.\n", ntohl(current->inResult), ntohl(clcProt->inResult));
        if (ntohl(current->inResult) == ntohl(clcProt->inResult))
        {
          //Correct
          clcMsg.message = htonl(1);
          printf("OK.\n");
        }
        else
        {
          //Wrong
          clcMsg.message = htonl(2);
          printf("NOT OK.\n");
        }
      }
      else
      {
        printf("float result.\nServer awnser: %f.\nClient awnser: %f.\n", current->flResult, clcProt->flResult);
        if (abs(current->flResult - clcProt->flResult) < 0.0001)
        {
          //correct
          printf("OK.\n");
          clcMsg.message = htonl(1);
        }
        else
        {
          //Wrong
          clcMsg.message = htonl(2);
          printf("NOT OK.\n");
        }
      }
    }
    else
    {
      //Wrong client
      clcMsg.message = htonl(2);
    }
  }
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
  initCalcLib();

  int rv = 0;
  int yes = 1;
  int bytesRcv = 0;
  int bytesSent = 0;

  char ipPort[250];

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in clientAddr;

  calcProtocol clcProt;
  calcMessage clcMsg;
  calcMessage *calcMsgP = (calcMessage *)malloc(sizeof(calcMessage));

  calcProtocol *calcProtP = (calcProtocol *)malloc(sizeof(calcProtocol));

  void *structP = malloc(sizeof(calcProtocol));

  clientInfo client;
  client.id = 0;
  client.next = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  rv = getaddrinfo(Desthost, Destport, &hints, &servinfo);
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

  socklen_t clientLen = sizeof(clientAddr);

  while (run)
  {
    memset(&clcMsg, 0, sizeof(clcMsg));
    memset(&clientAddr, 0, sizeof(clientAddr));

    bytesRcv = recvfrom(sockfd, (void *)structP, MAXLINE, 0, (struct sockaddr *)&clientAddr, &clientLen);
    if (bytesRcv < 0)
    {
      printf("Could not recive.\n");
      exit(1);
    }
    //Recived a calcMessage
    if (bytesRcv == sizeof(calcMessage))
    {
      printf("Recived calcMessage.\n");
      //If the calcMessage is correct send back a calcProtocol
      calcMsgP = (calcMessage *)structP;
      // type = 22, message = 0, protocol = 17, major_version = 1, minor_version = 0).
      if (calcMsgP->type == ntohs(22) &&
          calcMsgP->message == ntohl(0) &&
          calcMsgP->protocol == ntohs(17) &&
          calcMsgP->major_version == ntohs(1) &&
          calcMsgP->minor_version == ntohs(0))
      {
        //Correct calcMessage
        memset(&clcProt, 0, sizeof(clcProt));
        clcProt.arith = htonl(getArith());
        printf("Arit nr: %d.\n", ntohl(clcProt.arith));
        //clcProt.id = id++;
        if (ntohl(clcProt.arith) < 0)
        {
          printf("Could not get random arith.\n");
          continue;
        }
        else if (ntohl(clcProt.arith) < 5)
        {
          //int
          clcProt.inValue1 = htonl(randomInt());
          clcProt.inValue2 = htonl(randomInt());
          printf("Gave int values: %d and %d.\n", ntohl(clcProt.inValue1), ntohl(clcProt.inValue2));
        }
        else if (ntohl(clcProt.arith) >= 5)
        {
          //float
          clcProt.flValue1 = randomFloat();
          clcProt.flValue2 = randomFloat();
          printf("Gave float values: %f and %f.\n", clcProt.flValue1, clcProt.flValue2);
        }
        clcProt.id = htonl(id++);
        bytesSent = sendto(sockfd, &clcProt, sizeof(clcProt), 0, (struct sockaddr *)&clientAddr, clientLen);
        //Save client
        addClient(&client, clcProt, clientAddr);
      }
      else
      {
        printf("Wrong protocol.\n");
        //set calcMsgP //type=2, msg=2, maj_v=1, min_v=0
        calcMsgP->type = 2;
        calcMsgP->message = 2;
        calcMsgP->major_version = 1;
        calcMsgP->minor_version = 0;

        //Send back calcMsgP
        bytesSent = sendto(sockfd, calcMsgP, sizeof(calcMsgP), 0, (struct sockaddr *)&clientAddr, clientLen);
      }
    }
    //Recived a calcProtocol
    else if (bytesRcv == sizeof(calcProtocol))
    {
      printf("Recived calcProtocol.\n");
      calcProtP = (calcProtocol *)structP;

      //check if the awnser is correct
      sprintf(ipPort, "%s:%d", inet_ntoa(clientAddr.sin_addr), htons(clientAddr.sin_port));
      checkJob(&client, ipPort, calcProtP, clcMsg);
      bytesSent = sendto(sockfd, (calcMessage *)&clcMsg, sizeof(clcMsg), 0, (struct sockaddr *)&clientAddr, clientLen);
    }

    printf("This is the main loop, %d time.\n", loopCount);
    sleep(1);
    loopCount++;
  }

  return (0);
}
