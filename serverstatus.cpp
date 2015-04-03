#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <limits>

#include "system_stats.h"
#include "unix_functions.h"
#include "config.h"

using namespace std;


//===================================================================================
// CONFIGURATION SECTION
//===================================================================================

// Version to check with possibly incompatible config files
#define VERSION "v0.4-beta"



// After one day the counter resets (this is the maximum interval too)
#define MAX_TIME 1440

// Loop-Time: The loop will restart every X seconds
// Note: Some calculations within the loop may take some time.
//       There is no guarantee that loop times smaller than a few seconds
//       archieved.
// Note: In case of change the interval values need adjustments.
// Default: 60 seconds
#define LOOP_TIME 60


// Per default ServerStatus will look for a configuration file at these positions:
#define PATHC 2
const string PATH[] = {"/usr/local/etc/serverstatus.cfg", "/etc/serverstatus.cfg"};


// Defines which systat classes will be created (a configuration file with 
// working commands is still required!)
#define SYS_COUNT 6
const status SYS_TYPE[] = {CPU, Load, HDD, Mount, Memory, Network};



//===================================================================================
// END OF CONFIGURATION SECTION
//===================================================================================




// struct that contains to the interval time to each command
struct sys_stat_t {
  vector<int> interval;
  vector<SystemStats*> stat;
};
typedef struct sys_stat_t sys_stat;





// writes the path to the config file into the submitted parameter:
bool getConfigFilePath(string &output) {
  for (int i = 0; i < PATHC; i++) {
    if (FileExists(PATH[i])) {
      output = PATH[i];
      return true;
    }
  }
  return false;
}



//===================================================================================
// THE MAIN MODE:
// This creates a daemon process that will keep running in the background
// and create json files as output.
// Logging via syslog service possible.
//===================================================================================
void startDaemon(const string &configFile) {
  int userID = getuid();
  
  // check of an instance of serverstatus is already running in the background
  string cmd = "pgrep serverstatus";
  string did = getCmdOutput(&cmd[0]);
  if ((did != "") && (atoi(did.c_str()) != getpid())) {
    printf("Daemon is running already. \n");
    exit(EXIT_FAILURE);
  }
  // check for root privileges
  if (userID != 0) {
    printf("\nWarning: ServerStatus is currently not running with root privileges. \nServerStatus itself does not need root privileges but if you use any commands that do need require those there might occur some problems. \nIf you encounter any problems try restarting ServerStatus as root. \n\n\n");
    }


  pid_t pid, sid;
  pid = fork();

  // could not create child process
  if (pid < 0) { printf("Starting ServerStatus: [failed] \n"); exit(EXIT_FAILURE); }

  // child process created: terminate parent
  if (pid > 0) { printf("Starting ServerStatus: [successful]\n"); exit(EXIT_SUCCESS); }

  umask(0);

  // using syslog local1 for this daemon
  //setlogmask(LOG_UPTO (LOG_NOTICE));
  openlog("ServerStatus", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  syslog(LOG_NOTICE, "Started by User %d", userID);
  if (userID != 0) {
    syslog(LOG_WARNING, "Warning: ServerStatus is currently not running with root privileges. \nServerStatus itself does not need root privileges but if you use any commands that do need require those there might occur some problems. \nIf you encounter any problems try restarting ServerStatus as root.");
  }

  sid = setsid();
  if (sid < 0) {
    syslog (LOG_ERR, "Error: Could not create new sid for child process");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "New SID for child process created.");


  if ((chdir("/")) < 0) {
    syslog (LOG_ERR, "Error: Could not change working directory.");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "Changed working directory to root directory.");


  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);


  
  // variables
  int i = 0;
  int elapsedTime = 0;

  time_t startTime,endTime;

  config *configuration = new config(configFile);
  syslog(LOG_DEBUG, "Configuration file loaded.");
  
  
  // Version check
  if (configuration->readVersion() != VERSION) {
    throw "Configuration version does not match.";
    syslog (LOG_ERR, "Error: Configuration version does not match.");
    exit(EXIT_FAILURE);
  }
  

  sys_stat sys;
  for (int j = 0; j < SYS_COUNT; j++) {
    // read interval time and create class for each at top defined status type
    int intervalTime = configuration->readInterval(getSectionFromType(SYS_TYPE[j]).c_str());
    if (configuration->readEnabled(getSectionFromType(SYS_TYPE[j]).c_str()) == false) {
      intervalTime = 0;
    }
    sys.interval.push_back(intervalTime);
    sys.stat.push_back(new SystemStats(SYS_TYPE[j], configFile));
    sys.stat[j]->loadFromFile(); 
  }
  syslog(LOG_DEBUG, "sys_stat objects created.");
		
  // the loop fires once every minute
  while(1) {
	
    // get the duration of function calling...
    startTime = clock();
    
    // for each status type read status if interval time is reached
    for (int j = 0; j < SYS_COUNT; j++) {
      // if interval = 0 -> disabled
      if ((sys.interval[j] != 0)) {
        if (i % sys.interval[j] == 0) { 
          sys.stat[j]->readStatus(); 
        }
      }
    }
	 
    // update counter
    if (i < MAX_TIME) { i++; } else { i = 0; }
			
    syslog (LOG_DEBUG, "loop no. %d finished", i);
			
    // now calculate how long we have to sleep
    endTime = clock();
    elapsedTime = (endTime - startTime)/CLOCKS_PER_SEC;
			
    sleep(LOOP_TIME - elapsedTime);  // sleep the remaining time
  }
   
  closelog();
}


void stopDaemon() {
  bool killed = false;
  string cmd = "pgrep serverstatus";
  vector<string> output = split(getCmdOutput(&cmd[0]), 10);
  for (int i = 0; i < output.size(); i++) {
    if  (atoi(output[i].c_str()) != getpid()) {
      string killcmd = "kill " + output[i];
      system(killcmd.c_str());
      killed = true;
      printf("ServerStatus [%s] stopped. \n", output[i].c_str());
    }
  }
  
  if (!killed) {
    printf("ServerStatus is currently not running on your system. \n");
  }
}



void getDaemonStatus() {
  bool running = false;
  string cmd = "pgrep serverstatus";
  vector<string> output = split(getCmdOutput(&cmd[0]), 10);
  for (int i = 0; i < output.size(); i++) {
    if  (atoi(output[i].c_str()) != getpid()) {
      running = true;
      printf("ServerStatus is currently running with pid %s. \n", output[i].c_str());
    }
  }
  
  if (running == false) {
    printf("ServerStatus is currently not running. \n");
  }
}





//===================================================================================
// INPUT HANDLING:
// Parse command line paramter and execute according functions.
//===================================================================================
int main(int argc, char *argv[]) {
  
  // Load configuration
  string _configpath;
  if (!getConfigFilePath(_configpath)) {
    printf("Could not find a configuration file. \nMake sure your configuration file is \"%s\" or \"%s\". \n", PATH[0].c_str(), PATH[1].c_str());
    exit(EXIT_FAILURE);
  }

  
  /***************************************************************
  *** ServerStatus differentiates between two modes:
  ***  1) The main mode is starting without any parameters:
  ***     It then creates a daemon that keeps running in the
  ***     background until the OS shuts down or the process
  ***     is killed
  ***  2) The secound mode is starting with paramteres:
  ***     Right now these are:
  ***      "serverstatus --help" or "serverstatus -h"
  ***      "start", "restart" and "stop" 
  ***      "status"
  ***************************************************************/ 
  
  if ((argc == 0) || ((argc > 0) && (strcmp(argv[1],"start") == 0))) {
    // MODE 1: run the daemon
    startDaemon(_configpath);

  } else if (argc > 0) {
    // MODE 2: parse the options: 

    if ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) {
      // Show help:
      system("man serverstatus");
      exit(EXIT_SUCCESS);
    }

    else if (strcmp(argv[1], "stop") == 0) {
      // stop other running instances:
      stopDaemon();
      exit(EXIT_SUCCESS);
    }


    else if (strcmp(argv[1], "restart") == 0) {
      // stop and start serverstatus again:
      stopDaemon();
      startDaemon(_configpath);
    }
    
    else if (strcmp(argv[1], "status") == 0) {
      // return status
      getDaemonStatus();
    }
    
    if ((strcmp(argv[1], "--config-check") == 0) || (strcmp(argv[1], "-c") == 0)) {
      // Check configuration file
      config *configuration = new config(_configpath);
      configuration->showErrorLog();
      configuration->performSecurityCheck(_configpath);
    }
    
    else {
      printf("command line parameter not recognised. \nUse serverstatus --help to see all possible commands.\n");
    }
  }
}