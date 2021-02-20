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
int id = 1;

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
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}
int getArith()
{

  char *randArith = randomType();
  int ret = -1;
  if (randArith == "add")
    ret = 1;
  else if (randArith == "sub")
    ret = 2;
  else if (randArith == "mul")
    ret = 3;
  else if (randArith == "div")
    ret = 4;
  else if (randArith == "fadd")
    ret = 5;
  else if (randArith == "fsub")
    ret = 6;
  else if (randArith == "fmul")
    ret = 7;
  else if (randArith == "fdiv")
    ret = 8;

  return ret;
}

struct clientInfo
{
  int id;
  int arith;
  int inValue1, inValue2, inResult;
  float flValue1, flValue2, flResult;
  char clientAddress[250];//ip:port
  struct clientInfo *next;
};
void addClient(clientInfo *client, calcProtocol &clcProt, addrinfo &p)
{
  clientInfo *current = client;
  while (current->next != NULL)
  {
    current = current->next;
  }

  //Get client ip and port and put in cliendAddress char array/////////////////////////////////////////////
  
  current->next->id = clcProt.id;
  current->next->arith = clcProt.arith;
  current->next->inValue1 = clcProt.inValue1;
  current->next->inValue2 = clcProt.inValue2;
  current->next->flValue1 = clcProt.flValue1;
  current->next->flValue2 = clcProt.flValue2;

  //Calculate andsave the result
  switch (current->next->arith)
  {
  case 1:
    current->next->inResult = current->next->inValue1 + current->next->inValue2;
    break;
  case 2:
    current->next->inResult = current->next->inValue1 - current->next->inValue2;
    break;
  case 3:
    current->next->inResult = current->next->inValue1 * current->next->inValue2;
    break;
  case 4:
    current->next->inResult = current->next->inValue1 / current->next->inValue2;
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
    else if(current->next == NULL){
      searching = false;
    }
    else{
      current = current->next;
    }
  }
  if(found){
    //Ceck if it is the right client
    
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

  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in clientAddr;
  

  struct hostent *hostp[MAXCLIENTS]; //client host info

  calcProtocol clcProt;
  calcMessage clcMsg;
  calcMessage *calcMsgP = (calcMessage *)malloc(sizeof(calcMessage));

  calcProtocol *calcProtP = (calcProtocol *)malloc(sizeof(calcProtocol));

  void *structP = malloc(sizeof(calcProtocol));

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
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
    //////////////////////////////////////////////////////////////////////////memset(&clientAddr, 0, sizeof(clientAddr));

    bytesRcv = recvfrom(sockfd, &structP, sizeof(structP), NULL, p->ai_addr, &p->ai_addrlen);
    if (bytesRcv < 0)
    {
      printf("Could not recive.\n");
      exit(1);
    }
    //Recived a calcMessage
    if (bytesRcv == sizeof(calcMessage))
    {
      //If the calcMessage is correct send back a calcProtocol
      calcMsgP = (calcMessage *)structP;
      // type = 22, message = 0, protocol = 17, major_version = 1, minor_version = 0).
      if (calcMsgP->type == 22 &&
          calcMsgP->message == 0 &&
          calcMsgP->protocol == 17 &&
          calcMsgP->major_version == 1 &&
          calcMsgP->minor_version == 0)
      {
        //Correct calcMessage
        memset(&clcProt, 0, sizeof(clcProt));
        clcProt.arith = getArith();
        clcProt.id = id++;
        if (clcProt.arith < 0)
        {
          printf("Could not get random arith.\n");
          continue;
        }
        else if (clcProt.arith < 5)
        {
          //int
          clcProt.inValue1 = randomInt();
          clcProt.inValue2 = randomInt();
        }
        else if (clcProt.arith >= 5)
        {
          //float
          clcProt.flValue1 = randomFloat();
          clcProt.flValue2 = randomFloat();
        }
        bytesSent = sendto(sockfd, &clcProt, sizeof(clcProt), 0, p->ai_addr, p->ai_addrlen);
        if (bytesSent < 0)
        {
          printf("Recive error.\n");
          continue;
        }
      }
      else
      {
        //set calcMsgP //type=2, msg=2, maj_v=1, min_v=0
        //Send back calcMsgP
      }
      //store this client somehow??
    }
    //Recived a calcProtocol
    else if (bytesRcv == sizeof(calcProtocol))
    {
      calcProtP = (calcProtocol *)structP;
      //check if the awnser is correct
    }

    printf("This is the main loop, %d time.\n", loopCount);
    sleep(1);
    loopCount++;
  }

  return (0);
}
