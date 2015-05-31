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
#include <fstream>
#include <limits>
#include <signal.h>
#include <unistd.h>

#include <pthread.h>
#include <mutex>

#include "serverstatus.h"
#include "system_stats.h"
#include "unix_functions.h"
#include "config.h"
#include "communication.h"

using namespace std;


//===================================================================================
// CONFIGURATION SECTION
//===================================================================================

// Version to check with possibly incompatible config files
#define VERSION "v0.5-beta"

// location where the pid file shall be stored
#define PID_FILE "/var/run/serverstatus.pid"

#define MAGIC_NUMBER "-42"



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




//===================================================================================
// GLOBAL VARIABLE SECTION
//===================================================================================

volatile sig_atomic_t loop = 1;

pthread_mutex_t thread_Mutex = PTHREAD_MUTEX_INITIALIZER;

// this vector is misused as temporary stack for server-client exchange
vector<thread_value> thread_Val;

//===================================================================================
// END OF GLOBAL VARIABLE SECTION
//===================================================================================







// writes the path to the config file into the submitted parameter:
bool getConfigFilePath(string &output) {
  for (int i = 0; i < PATHC; i++) {
    if (file_exists(PATH[i])) {
      output = PATH[i];
      return true;
    }
  }
  return false;
}


// reads a pid file without(!) checking its existence 
string read_pid_file(const string& name) {
  ifstream in(name);
  string l;

  if (getline(in, l)) {
    return l;
  } else {
    return MAGIC_NUMBER;
  }
}

// write pid file. returns false if an error occured
bool write_pid_file(const string& name) {
  ofstream of;
  of.open(PID_FILE);
  if (of.fail()) {
    return false;
  } else {
    of << getpid();
    of.close();
    return true;
  }
}

// checks if a process is running
bool pid_running(int pid) {
  return (0 == kill(pid, 0));
}


// used for SIGTERM handling
void terminate(int signum) {
  loop = 0;
}



void storeValueGlobal(vector<string> value) {
  // value must at least consist out of a type, a id and one value
  if (value.size() <= 3) { return; }
  
  // store value in global variable (enter mutex area)
  pthread_mutex_lock(&thread_Mutex);
  
  // iterate through all currently stored values and check if this type already exists
  int k = 0;
  int id = -1;
  while ((k < thread_Val.size()) && (id == -1)) {
    if ((getTypeFromString(value[0]) == thread_Val[k].type) && (value[1] == thread_Val[k].clientID)) {
      id = k;
    }
    k++;
  }
  
  thread_value t;
  if (id == -1) {
    // create new entry
    t.type = getTypeFromString(value[0]);
    t.clientID = trim(value[1]);
    for (int i = 2; i < value.size(); i++) {
      t.value.push_back(atof(trim(value[i]).c_str()));
    }
    thread_Val.push_back(t);
  } else {
    // override existing entry
    thread_Val[id].value.clear();
    for (int i = 2; i < value.size(); i++) {
      thread_Val[id].value.push_back(atof(trim(value[i]).c_str()));
    }
  }
  
  // leave mutex
  pthread_mutex_unlock(&thread_Mutex);
}


thread_value readValueGlobal(status type, string clientID) {
  
  pthread_mutex_lock(&thread_Mutex);
  
  thread_value s;
  s.type = type;
  // s.value stays empty if non is found
  
  for (int i = 0; i < thread_Val.size(); i++){
    if ((type == thread_Val[i].type) && (clientID == thread_Val[i].clientID)) {
      // copy values into local variable
      for (int j = 0; j < thread_Val[i].value.size(); j++) {
        s.value.push_back(thread_Val[i].value[j]);
      }
      
      // delete "read" entry from global variable
      thread_Val[i].value.clear();
    }
  } 
  
  pthread_mutex_unlock(&thread_Mutex);
  
  // return struct
  return s;
}


//===================================================================================
// SERVER THREAD:
// creates a thread that waits and listens on a socket for external input
// which is then stored in a global variable (!MUTEX)
//===================================================================================
void *serverThread(void *port) {
  std::string* val = reinterpret_cast<std::string*>(port);
  syslog(LOG_DEBUG, "Server thread started; Listening at port %s", (*val).c_str());
  
  int sockfd = bind_socket(*val);
  if (sockfd == 0) { pthread_exit(0); }
  
  string input;
  while (loop) {
    try {
      // wait for input on the socket
      input = read_from_socket(sockfd);
      
      // string is expected to have form such as "type, id, value1, value2, ..."
      vector<string> s = split(input, ',');
      storeValueGlobal(s);
      
    } catch (int e) {
      
    }
  }
  
  close(sockfd);
  
  pthread_exit(0);
}

//===================================================================================
// THE MAIN MODE:
// This creates a daemon process that will keep running in the background
// and create json files as output.
// Logging via syslog service possible.
//===================================================================================
void startDaemon(const string &configFile) {
  int userID = getuid();
  
  // check for root privileges
  if (userID != 0) {
    printf("Error: ServerStatus requires root privileges to work properly.\nTry to run serverstatus as root.\n\n");
    exit(EXIT_FAILURE);
  }
  
  // check for other instances of serverstatus
  if (getDaemonStatusRunning(false)) {
    printf("Error: ServerStatus is already running. \n");
    exit(EXIT_FAILURE);
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
  
  
  // create pid file
  if (!write_pid_file(PID_FILE)) {
    syslog (LOG_ERR, "Error: pid file could not be created.");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "pid file successfully written.");
  

  // set sid
  sid = setsid();
  if (sid < 0) {
    syslog (LOG_ERR, "Error: Could not create new sid for child process.");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "New SID for child process created.");


  // change working directory to root dir
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
  
  
  // SIGTERM handling
  struct sigaction term;
  memset(&term, 0, sizeof(struct sigaction));
  term.sa_handler = terminate;
  sigaction(SIGTERM, &term, NULL);
  syslog(LOG_DEBUG, "SIGTERM handling added.");
  
  
  // handle server/client mode
  pthread_t thread;
  pthread_attr_t thread_attr;
  
  if (configuration->readApplicationType() == "server") {
    // server requires an additional thread that handles external input
    string *port = new string(configuration->readServerPort());
    
    pthread_attr_init(&thread_attr);
    pthread_create(&thread, &thread_attr, serverThread, (void *)port);
  }
  

  // sys_stat classes
  sys_stat sys;
  for (int j = 0; j < SYS_COUNT; j++) {
    // read interval time and create class for each at top defined status type
    int intervalTime = configuration->readInterval(getStringFromType(SYS_TYPE[j]).c_str());
    if (configuration->readEnabled(getStringFromType(SYS_TYPE[j]).c_str()) == false) {
      intervalTime = 0;
    }
    sys.interval.push_back(intervalTime);
    sys.stat.push_back(new SystemStats(SYS_TYPE[j], configFile));
    sys.stat[j]->loadFromFile(); 
  }
  syslog(LOG_DEBUG, "sys_stat objects created.");
		

  // the loop fires once every LOOP_TIME seconds
  while(loop) {
	
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
			
    syslog(LOG_DEBUG, "loop no. %d finished", i);
			
    // now calculate how long we have to sleep
    endTime = clock();
    elapsedTime = (endTime - startTime)/CLOCKS_PER_SEC;
			
    sleep(LOOP_TIME - elapsedTime);  // sleep the remaining time
  }
  
  remove(PID_FILE);
  syslog(LOG_NOTICE, "Process terminated.");
   
  closelog();
}


void stopDaemon() {
  // check for root privileges
  if (getuid() != 0) {
    printf("Error: root privileges are required to stop ServerStatus.\n\n");
    exit(EXIT_FAILURE);
  }
  
  // kill process if running
  if (getDaemonStatusRunning(false)) {
    string pid = read_pid_file(PID_FILE);
    if (pid != MAGIC_NUMBER) {
      if (kill(stoi(pid), SIGTERM) == 0) {
        // could be killed -> delete pid file (if not already deleted by terminated process)
        //remove(PID_FILE);
        printf("ServerStatus [%s] was successfully stopped.\n", pid.c_str());
      } else {
        printf("Error: ServerStatus could not be stopped.\n");
      }
    } else {
      printf("ServerStatus is currently not running.");
    }
  } else {
    printf("ServerStatus is currently not running.");
  }
}




bool getDaemonStatusRunning(bool output) {
  bool result = false;
  char msg[100];
  // check for pid file
  if (file_exists(PID_FILE)){
    string pid = read_pid_file(PID_FILE);
    
    if (pid != MAGIC_NUMBER) {
      // check if process is still running
      if (getuid() == 0) {
        if (pid_running(stoi(pid.c_str()))) {
          sprintf(msg, "ServerStatus is currently running with pid %s. \n", pid.c_str());
          result = true;
        } 
      } else {
        sprintf(msg, "ServerStatus might be running with pid %s. \nRun as root to get more precise information. \n", pid.c_str());
        result = true;
      }
    } 
  } 
    
  if (output) {
    if (!result) {
      sprintf(msg, "ServerStatus is currently not running.\n");
    }
    printf("%s", msg);
  }
  return result;
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
      getDaemonStatusRunning(true);
    }
    
    else if ((strcmp(argv[1], "--config-check") == 0) || (strcmp(argv[1], "-c") == 0)) {
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