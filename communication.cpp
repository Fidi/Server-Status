#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <pthread.h>
#include <mutex>
#include <syslog.h>
#include <vector>

#include "serverstatus.h"
#include "communication.h"

#define MAXBUFLEN 100

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int bind_socket(string port) {
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  struct sockaddr_storage;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
    syslog(LOG_ERR, "Server thread: getaddrinfo - %s.", gai_strerror(rv));
    return 0;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
    }
    
    if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    syslog(LOG_ERR, "Server thread: Failed to bind socket.");
    return 0;
  }

  freeaddrinfo(servinfo);
  
  return sockfd;
}




string read_from_socket(int sockfd) {
  struct sockaddr_storage their_addr;
  char buf[MAXBUFLEN];
  int numbytes;
  socklen_t addr_len;
  char s[INET6_ADDRSTRLEN];

  addr_len = sizeof their_addr;
  if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    exit(1);
  }

  syslog(LOG_NOTICE, "Server thread: Incoming data from %s.", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
  
  buf[numbytes] = '\0';
  
  return string(buf);
}



void write_to_socket(string server_ip, string port, string msg) {
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  if ((rv = getaddrinfo(server_ip.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
    return;
  }

  // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
    }

    break;
  }

  if (p == NULL) {
    return;
  }

  if ((numbytes = sendto(sockfd, msg.c_str(), strlen(msg.c_str()), 0, p->ai_addr, p->ai_addrlen)) == -1) {
    exit(1);
  }

  freeaddrinfo(servinfo);

  syslog(LOG_NOTICE, "Sent %d bytes to %s.", numbytes, server_ip.c_str());
  close(sockfd);
}