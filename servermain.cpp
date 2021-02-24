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
//int loopCount = 0;
bool run = true;
int id = 1;
int nrOfClients = 0;
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
clientInfo *firstClient = nullptr; //Gör pekare

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum)
{
  // As anybody can call the handler, its good coding to check the signal number that called it.

  time_t end = time(0);
  clientInfo *current = firstClient;
  clientInfo *prev = firstClient;
  int nrOfRemoved = 0;
  int loopCount = 0;
  if (nrOfClients > 0)
  {
    printf("Number of clients: %d\n", nrOfClients);
    while (current != nullptr)
    {
      loopCount++;
      if ((end - current->start) >= 10)
      {
        printf("we need to remove the client with ID: %d\n", ntohl(current->id));
        if (current == firstClient)
        {
          //Fisrt client

          if (nrOfClients > 1)
          {
            firstClient = current->next;
          }
          //We only have one client
          else
          {
            firstClient = nullptr;
          }
          delete current;
        }
        else if (current->next == nullptr)
        {
          //Last client
          delete current;
        }
        else
        {
          //A client in the middle
          prev->next = current->next;
          delete current;
        }
        nrOfClients--;
      }
      prev = current;
      current = current->next;
    }
    printf("loop count: %d.\n", loopCount);
  }
  else
  {
    printf("No clients.\n");
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

void addClient(calcProtocol &clcProt, struct sockaddr_in &clientAddress)
{
  clientInfo *current = firstClient;

  if (current == nullptr)
  {
    printf("First cilet.\n");
    current = new clientInfo;
    firstClient = current;
  }
  else
  {
    while (current->next != nullptr)
    {
      current = current->next;
    }
    current->next = new clientInfo;
    current = current->next;
    printf("Another boring client.\n");
  }

  //current->next = (clientInfo *)malloc(sizeof(clientInfo));
  sprintf(current->clientAddress, "%s:%d", inet_ntoa(clientAddress.sin_addr), htons(clientAddress.sin_port));
  nrOfClients++;
  current->start = time(0);
  current->id = clcProt.id;
  current->arith = clcProt.arith;
  current->inValue1 = clcProt.inValue1;
  current->inValue2 = clcProt.inValue2;
  current->flValue1 = clcProt.flValue1;
  current->flValue2 = clcProt.flValue2;

  //Calculate andsave the result
  switch (ntohl(current->arith))
  {
  case 1:
    current->inResult = ntohl(current->inValue1) + ntohl(current->inValue2);
    break;
  case 2:
    if (ntohl(current->inValue1) >= ntohl(current->inValue2))
    {
      current->inResult = ntohl(current->inValue1) - ntohl(current->inValue2);
    }
    else
    {
      current->inResult = ntohl(current->inValue2) - ntohl(current->inValue1);
    }

    break;
  case 3:
    current->inResult = ntohl(current->inValue1) * ntohl(current->inValue2);
    break;
  case 4:
    current->inResult = ntohl(current->inValue1) / ntohl(current->inValue2);
    break;
  case 5:
    current->flResult = current->flValue1 + current->flValue2;
    break;
  case 6:
    current->flResult = current->flValue1 - current->flValue2;
    break;
  case 7:
    current->flResult = current->flValue1 * current->flValue2;
    break;
  case 8:
    current->flResult = current->flValue1 / current->flValue2;
    break;

  default:
    break;
  }
  //Convert to network byte order
  if (ntohl(current->arith) < 5)
  {
    current->inResult = htonl(current->inResult);
  }

  current->next = nullptr;
}
void checkJob(char *Iaddress, calcProtocol *clcProt, calcMessage &clcMsg)
{
  clientInfo *current = firstClient;
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
        //printf("int result.\nServer awnser: %d.\nClient awnser: %d.\n", ntohl(current->inResult), ntohl(clcProt->inResult));
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
        //printf("float result.\nServer awnser: %f.\nClient awnser: %f.\n", current->flResult, clcProt->flResult);
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
  else
  {
    printf("Did not find this client.\n");
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
      //printf("Recived calcMessage.\n");
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
        //printf("Arit nr: %d.\n", ntohl(clcProt.arith));
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
          //printf("Gave int values: %d and %d.\n", ntohl(clcProt.inValue1), ntohl(clcProt.inValue2));
        }
        else if (ntohl(clcProt.arith) >= 5)
        {
          //float
          clcProt.flValue1 = randomFloat();
          clcProt.flValue2 = randomFloat();
          //printf("Gave float values: %f and %f.\n", clcProt.flValue1, clcProt.flValue2);
        }
        clcProt.id = htonl(id++);
        bytesSent = sendto(sockfd, &clcProt, sizeof(clcProt), 0, (struct sockaddr *)&clientAddr, clientLen);
        //Save client
        addClient(clcProt, clientAddr);
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
      if (nrOfClients > 0)
      {
        //printf("Recived calcProtocol.\n");
        calcProtP = (calcProtocol *)structP;

        //check if the awnser is correct
        sprintf(ipPort, "%s:%d", inet_ntoa(clientAddr.sin_addr), htons(clientAddr.sin_port));
        checkJob(ipPort, calcProtP, clcMsg);
        bytesSent = sendto(sockfd, (calcMessage *)&clcMsg, sizeof(clcMsg), 0, (struct sockaddr *)&clientAddr, clientLen);
      }
      else{
        printf("Dont even try.\n");
      }
    }
  }
  return (0);
}
