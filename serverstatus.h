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

// struct for client server exchange
struct thread_value_t {
	std::string type;
	std::vector<double> value;
};
typedef struct thread_value_t thread_value;



void startDaemon(const std::string &configFile);
void stopDaemon();
bool getDaemonStatusRunning(bool output);


#endif