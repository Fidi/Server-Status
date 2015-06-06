#ifndef _communication_hpp_
#define _communication_hpp_

// OpenSSL should always be optional in case it is not installed
#define __OPENSSL__ true

#include <string.h>
#include <mutex>


#if __OPENSSL__
	#include <openssl/ssl.h>
	#include <openssl/err.h>
#endif



enum app_mode_t {
  CLIENT,
  SERVER
};
typedef enum app_mode_t app_mode;


struct connection_t {
  int socket;
  #if __OPENSSL__
  SSL_CTX *ctx;
  #endif
};
typedef struct connection_t connection;



/*****************************************************************
**
**  PUBLIC FUNCTIONS
**
*****************************************************************/

// creates a connection (with optional SSL encryption)
connection create_socket(app_mode mode, int port, std::string host_ip, bool ssl = true);

// closes the connection
void destroy_socket(connection &con);

// if SSL then the Server should load the certificates
void load_local_certificate(connection &con, char* CertFile, char* KeyFile);

// server function: reads input from socket
std::string read_from_socket(connection &con);

// client function: writes output to socket
void write_to_socket(connection &con, std::string msg);


#endif