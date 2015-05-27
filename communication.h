#ifndef _communication_hpp_
#define _communication_hpp_

#include <string.h>
#include <mutex>

#include "serverstatus.h"



int bind_socket(std::string port);
std::string read_from_socket(int sockfd);
void write_to_socket(std::string server_ip, std::string port, std::string msg);



#endif