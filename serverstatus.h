#ifndef _serverstatus_hpp_
#define _serverstatus_hpp_

#include <vector>

#include "system_stats.h"


// struct that contains to the interval time to each command
struct sys_stat_t {
  std::vector<int> interval;
  std::vector<SystemStats*> stat;
};
typedef struct sys_stat_t sys_stat;



struct server_thread_t {
  int port;
  bool ssl;
  char* cert_file;
  char* key_file;
};
typedef struct server_thread_t server_thread;


void storeValueGlobal(std::vector<std::string> value);
thread_value readValueGlobal(std::string section, std::string clientID);



void startDaemon(const std::string &configFile);
void stopDaemon();
bool getDaemonStatusRunning(bool output);


#endif