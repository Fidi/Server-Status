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
	status type;									//what information shall be stored
	std::string clientID;					//random identification number that has to match
	std::vector<double> value;   //values to be stored
};
typedef struct thread_value_t thread_value;



void storeValueGlobal(std::vector<std::string> value);
thread_value readValueGlobal(status type, std::string clientID);



void startDaemon(const std::string &configFile);
void stopDaemon();
bool getDaemonStatusRunning(bool output);


#endif