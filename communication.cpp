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
#include <syslog.h>
#include <vector>

#include "communication.h"

#if __OPENSSL__
  #include "openssl/ssl.h"
  #include "openssl/err.h"
#endif

// number of queued connection requests
#define BACKLOG 10

#define MAXBUFLEN 100

using namespace std;


/*****************************************************************
**
**  PRIVATE FUNCTION DECLARATIONS 
**
*****************************************************************/
int listen_on_tcp(int port);
int connect_to_tcp(int port, string host_ip);

void *get_in_addr(struct sockaddr *sa);

string read_input(int sockfd);
string read_tcp_input(int sockfd);

void write_output(int sockfd, string msg);
void write_tcp_output(int sockfd, string msg);

#if __OPENSSL__
  SSL_CTX* init_SSL_context(app_mode mode);
  bool show_remote_certificate(SSL* ssl);
  
  string read_input(int sockfd, SSL_CTX* ctx);
  string read_ssl_input(SSL* ssl);
  
  void write_output(int sockfd, string msg, SSL_CTX* ctx);
  bool write_ssl_output(SSL* ssl, string msg);
#endif






/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

// tries to create the according connection
connection create_socket(app_mode mode, int port, string host_ip, bool ssl) {
  connection c;
  if (mode == CLIENT) {
    c.socket = connect_to_tcp(port, host_ip);
  } else {
    c.socket = listen_on_tcp(port);
  }
  
  #if __OPENSSL__
    if (ssl) {
      c.ctx = init_SSL_context(mode);
    } else {
      c.ctx = NULL;
    }
  #endif
  
  return c;
}



// closes a socket and optionial ssl context
void destroy_socket(connection &con) {
  close(con.socket);
  
  #if __OPENSSL__
    if (con.ctx != NULL) {
      SSL_CTX_free(con.ctx);
    }
  #endif
}




bool read_from_socket(connection &con, string &input) {
  if (con.socket == 0) { return false; }
  
  #if __OPENSSL__
    input = read_input(con.socket, con.ctx);
  #else
    input = read_input(con.socket);
  #endif
  
  return true;
}



bool write_to_socket(connection &con, string msg) {
  if (con.socket == 0) { return false; }
  
  #if __OPENSSL__
    write_output(con.socket, msg, con.ctx);
  #else
    write_output(con.socket, msg);
  #endif
  
  return true;
}



/*****************************************************************
**
**  CONNECTION STUFF
**
*****************************************************************/

// open a tcp socket to listen to
int listen_on_tcp(int port) {
  int status;
  struct addrinfo hints, *servinfo, *l;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, to_string(port).c_str(), &hints, &servinfo)) != 0) {
    syslog(LOG_ERR, "Communication: getaddrinfo failed: %s", gai_strerror(status));
    return 0;
  }

  int sockfd;
  for(l = servinfo; l != NULL; l = l->ai_next) {
    if ((sockfd = socket(l->ai_family, l->ai_socktype, l->ai_protocol)) == -1) {
      continue;
    }

    if (::bind(sockfd, l->ai_addr, l->ai_addrlen) == -1) {
      close(sockfd);
      syslog(LOG_ERR, "Communication: Failed to bind socket.");
      continue;
    }

    break;
  }

  if (l == NULL) {
    syslog(LOG_ERR, "Communication: Creating socket failed.");
    return 0;
  }

  freeaddrinfo(servinfo);
  
  // start basic tcp listening
  if (listen(sockfd, BACKLOG) == -1) {
    close(sockfd);
    syslog(LOG_ERR, "Communication: Failed to listen on socket.");
    return 0;
  }
  
  return sockfd;
}



// connect to a tcp socket
int connect_to_tcp(int port, string host_ip) {
  int status;
  struct addrinfo hints,  *servinfo, *l;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; 
  hints.ai_socktype = SOCK_STREAM;
  
  
  if ((status = getaddrinfo(host_ip.c_str(), to_string(port).c_str(), &hints, &servinfo)) != 0) {
    syslog(LOG_ERR, "Communication: getaddrinfo failed: %s", gai_strerror(status));
    return 0;
  }


  int sockfd;
  for(l = servinfo; l != NULL; l = l->ai_next) {
    if ((sockfd = socket(l->ai_family, l->ai_socktype, l->ai_protocol)) == -1) {
      continue;
    }

    break;
  }

  if (l == NULL) {
    syslog(LOG_ERR, "Communication: Creating socket failed.");
    return 0;
  }


  if (connect(sockfd, l->ai_addr, l->ai_addrlen) == -1) {
    close(sockfd);
    syslog(LOG_ERR, "Communication: Failed to connect to socket.");
    return 0;
  }
  
  freeaddrinfo(servinfo);
  
  return sockfd;
}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



#if __OPENSSL__

  // initiates a ssl context
  SSL_CTX* init_SSL_context(app_mode mode) {
    SSL_CTX *ctx = NULL;

    #if OPENSSL_VERSION_NUMBER >= 0x10000000L
      const SSL_METHOD *method;
    #else
      SSL_METHOD *method;
    #endif

    // load/register all ssl stuff
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings(); 
    
    if (mode == SERVER) {
      // create new server-method instance
      method = SSLv23_method();
    } else {
      // create new client-method instance
      method = SSLv23_client_method();
    }
    
    // create the context based on the method
    ctx = SSL_CTX_new(method);
    
    if (ctx == NULL) {
      syslog(LOG_ERR, "Communication: Failed to create SSL context.");
      return NULL;
    }
    
    return ctx;
  }
  
#endif



/*****************************************************************
**
**  CERTIFICATE HANDLING (SSL only)
**
*****************************************************************/

// get local certificates
bool load_local_certificate(connection &con, char* CertFile, char* KeyFile) {
  #if __OPENSSL__

    if (con.ctx != NULL) {
      
      bool success = true;
      
      if (SSL_CTX_load_verify_locations(con.ctx, CertFile, KeyFile) != 1) {
        syslog(LOG_ERR, "Communication: Certificate - Verifiy Locations failed.");
      }
      if (SSL_CTX_set_default_verify_paths(con.ctx) != 1) {
        syslog(LOG_ERR, "Communication: Certificate - Verifiy Pathes failed.");
      }
      
      
      if (SSL_CTX_use_certificate_file(con.ctx, CertFile, SSL_FILETYPE_PEM) <= 0) {
        syslog(LOG_ERR, "Communication: Certificate - Use Certificate failed.");
        success = false;
      }
      if (SSL_CTX_use_PrivateKey_file(con.ctx, KeyFile, SSL_FILETYPE_PEM) <= 0) {
        syslog(LOG_ERR, "Communication: Certificate - Use Private Key failed.");
        success = false;
      }
      if (!SSL_CTX_check_private_key(con.ctx)){
        syslog(LOG_ERR, "Communication: Certificate - Private Key does not match.");
        success = false;
      }
      
      if (success) {
        syslog(LOG_NOTICE, "Communication: Certificates successfully loaded.");
        return true;
      } else {
        // if no certificate is found do not use SSL
        SSL_CTX_free(con.ctx);
        con.ctx = NULL;
        return false;
      }
    }
    
  #endif
  
  return false;
}


#if __OPENSSL__

  // get communication partner certificates
  bool show_remote_certificate(SSL* ssl) {
    X509 *cert;

    // try to get certificates
    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
      syslog(LOG_DEBUG, "Communication: Subject: %s", X509_NAME_oneline(X509_get_subject_name(cert), 0, 0));
      syslog(LOG_DEBUG, "Communication: Issuer:  %s", X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0));
      X509_free(cert);
      return true;
    }
    else {
      syslog(LOG_DEBUG, "Communication: No Certificates found.");
      return false;
    }
  }
  
#endif




/*****************************************************************
**
**  SERVER FUNCTIONS
**
*****************************************************************/

// this function will wait for input and return the moment something is received
string read_input(int sockfd) {
  return read_tcp_input(sockfd);
}


// read basic unencrypted tcp incoming traffic
string read_tcp_input(int sockfd) {
  struct sockaddr_storage their_addr;
  char buf[MAXBUFLEN];
  int numbytes;
  socklen_t addr_len;
  char s[INET6_ADDRSTRLEN];

  addr_len = sizeof their_addr;
  int client = accept(sockfd, (struct sockaddr*)&their_addr, &addr_len);
  if ((numbytes = recvfrom(client, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    syslog(LOG_ERR, "Communication: Failed to receive TCP input.");
  }

  syslog(LOG_NOTICE, "Communication: Incoming data from %s.", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
  
  buf[numbytes] = '\0';
  
  return string(buf);
}


#if __OPENSSL__

  // this function will wait for input and return the moment something is received
  string read_input(int sockfd, SSL_CTX* ctx) {
    string result;
    
    // if a valid ssl context is submitted, use it:  
    if (ctx != NULL) {
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      
      SSL *ssl;
      int client = accept(sockfd, (struct sockaddr*)&addr, &len);
      syslog(LOG_NOTICE, "Communication: Incoming data from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
      
      // create SSL state from context
      ssl = SSL_new(ctx);
      // activate SSL on connection
      SSL_set_fd(ssl, client);
      // and listen:
      result = read_ssl_input(ssl);
    } else {
      result = read_input(sockfd);
    }
    
    return result;
  }



  // read ssl encrypted incoming traffic
  string read_ssl_input(SSL* ssl) {   
    char buf[1024];
    int sd, numbytes;

    // do SSL-protocol accept
    if (SSL_accept(ssl) == -1) {
      syslog(LOG_ERR, "Communication: Failed to accept SSL connection.");
    } else {
      show_remote_certificate(ssl);
      numbytes = SSL_read(ssl, buf, sizeof(buf));
      if ( numbytes > 0 ) {
        buf[numbytes] = 0;
        syslog(LOG_DEBUG, "Communication: \"%s\"", buf);
      } else {
        syslog(LOG_ERR, "Communication: Error receiving SSL input.");
      }
    }
    
    // get socket connection
    sd = SSL_get_fd(ssl);
    // release SSL state
    SSL_free(ssl);
    // close connection
    close(sd);
    
    return string(buf);
  }
  
#endif





/*****************************************************************
**
**  CLIENT FUNCTIONS
**
*****************************************************************/

// write a message to the socket
void write_output(int sockfd, string msg) {
  write_tcp_output(sockfd, msg);
}


// write the message unencrypted 
void write_tcp_output(int sockfd, string msg) {
  int numbytes;
  if ((numbytes = send(sockfd, msg.c_str(), strlen(msg.c_str()), 0)) == -1) {
    return;
  }
}


#if __OPENSSL__

  // write a message to the socket
  void write_output(int sockfd, string msg, SSL_CTX* ctx) {
    if (ctx != NULL) {
      SSL *ssl;
      
      // create new SSL connection state
      ssl = SSL_new(ctx);
      // attach the socket descriptor
      SSL_set_fd(ssl, sockfd);
      
      if (!write_ssl_output(ssl, msg)) {
        write_output(sockfd, msg);
      }
    } else {
      write_output(sockfd, msg);
    }
  }
  
  
  // write the message encrypted 
  bool write_ssl_output(SSL* ssl, string msg) {
    // try to connect
    if (SSL_connect(ssl) == -1) {
      syslog(LOG_ERR, "Communication: Failed to connect to SSL socket.");
      return false;
    } else {   
      syslog(LOG_DEBUG, "Connected with %s encryption\n", SSL_get_cipher(ssl));
      
      if (show_remote_certificate(ssl)) {
        // encrypt message and send it
        SSL_write(ssl, msg.c_str(), strlen(msg.c_str()));
      } else {
        // release connection state
        SSL_free(ssl);
        return false;
      }
      
     
    }
    
    // release connection state
    SSL_free(ssl);
    return true;
  }

#endif