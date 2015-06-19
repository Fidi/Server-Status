#include "serverstatus.h"

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
#include <vector>

#include <pthread.h>
#include <mutex>

#include "system_stats.h"
#include "unix_functions.h"
#include "config.h"
#include "communication.h"
#include "status_types.h"

using namespace std;


//===================================================================================
// CONFIGURATION SECTION
//===================================================================================

// Version to check with possibly incompatible config files
#define VERSION "v0.6-beta"

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
const string PATH[] = {"/usr/local/etc/serverstatus.cfg", "/etc/serverstatus.cfg"};

//===================================================================================
// END OF CONFIGURATION SECTION
//===================================================================================




//===================================================================================
// GLOBAL VARIABLE SECTION
//===================================================================================

// this variable defines when all the loops (main loop, thread loops) shall terminate
volatile sig_atomic_t loop = 1;

pthread_mutex_t thread_Mutex = PTHREAD_MUTEX_INITIALIZER;

// this vector is misused as temporary stack for server-client exchange
vector<thread_value> thread_Val;

//===================================================================================
// END OF GLOBAL VARIABLE SECTION
//===================================================================================







// writes the path to the config file into the submitted parameter:
bool getConfigFilePath(string &output) {
  for (int i = 0; i < sizeof(PATH)/sizeof(PATH[0]); i++) {
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
    if ((value[0] == thread_Val[k].section) && (value[1] == thread_Val[k].clientID)) {
      id = k;
    }
    k++;
  }
  
  thread_value t;
  if (id == -1) {
    // create new entry
    t.section = value[0];
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


thread_value readValueGlobal(string section, string clientID) {
  
  pthread_mutex_lock(&thread_Mutex);
  
  thread_value s;
  s.section = section;
  // s.value stays empty if non is found
  
  for (int i = 0; i < thread_Val.size(); i++){
    if ((section == thread_Val[i].section) && (clientID == thread_Val[i].clientID)) {
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
void *serverThread(void *arg) {
  server_thread *s = (server_thread *)arg;
  
  syslog(LOG_NOTICE, "Server thread started; Listening at port %d", s->port);
  connection c = create_socket(SERVER, s->port, "127.0.0.1", s->ssl);
  
  
  // check if connection was created successfully 
  if (c.socket == -1) { 
    syslog(LOG_ERR, "Server Thread: Failed to create socket.");
    pthread_exit(0);
  }
  if ((s->ssl) && (s->cert_file[0] != '-') && (s->key_file[0] != '-')) {
    if (!load_local_certificate(c, s->cert_file, s->key_file)) {
      syslog(LOG_ERR, "Server Thread: Failed to load certificates.");
      pthread_exit(0);
    }
  }
  
  
  while (loop) {
    // wait for input on the socket
    string input;
    
    try {
      if (!read_from_socket(c, input)) {
        continue;
      }
      syslog(LOG_NOTICE, "Server Thread: Incoming data: %s", input.c_str());
      
      // string is expected to have form such as "type, id, value1, value2, ..."
      vector<string> s = split(input, ',');
      storeValueGlobal(s);
      
    } catch (int error) {
      syslog(LOG_ERR, "Server Thread: An error [%d] occurred.", error);
    }
  }
  
  destroy_socket(c);
  
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
  setlogmask(LOG_UPTO (LOG_NOTICE));
  openlog("ServerStatus", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  syslog(LOG_NOTICE, "Started by User %s", getUsernameFromUID(userID).c_str());
  
  
  // create pid file
  if (!write_pid_file(PID_FILE)) {
    syslog (LOG_ERR, "Main Thread: pid file could not be created.");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "Main Thread: pid file successfully written.");
  

  // set sid
  sid = setsid();
  if (sid < 0) {
    syslog (LOG_ERR, "Main Thread: Could not create new sid for child process.");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "Main Thread: New SID for child process created.");


  // change working directory to root dir
  if ((chdir("/")) < 0) {
    syslog (LOG_ERR, "Main Thread: Could not change working directory.");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "Main Thread: Changed working directory to root directory.");


  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);



  config *configuration = new config(configFile);
  syslog(LOG_DEBUG, "Main Thread: Configuration file loaded.");
  
  
  // Version check
  if (configuration->readVersion() != VERSION) {
    syslog (LOG_ERR, "Main Thread: Configuration version does not match.");
    exit(EXIT_FAILURE);
  }
  
  
  // SIGTERM handling
  struct sigaction term;
  memset(&term, 0, sizeof(struct sigaction));
  term.sa_handler = terminate;
  sigaction(SIGTERM, &term, NULL);
  syslog(LOG_DEBUG, "Main Thread: SIGTERM handling added.");
  
  
  // handle server/client mode
  pthread_t thread;
  pthread_attr_t thread_attr;
  if (configuration->readApplicationType() == "server") {
    // server requires an additional thread that handles external input
   // string *port = new string(configuration->readServerPort());
    server_thread s;
    s.port = configuration->readServerPort();
    s.ssl = configuration->readSSL();
    string cert = configuration->readCertFile().c_str();
    string key = configuration->readKeyFile().c_str();
    s.cert_file = &cert[0u];
    s.key_file = &key[0u];
    
    pthread_attr_init(&thread_attr);
    pthread_create(&thread, &thread_attr, serverThread, (void *)&s);
  }



  // get all the different sys_stat sections and create their classes
  vector<string> sys_sections = configuration->readSections();
  sys_stat sys;
  for (int i = 0; i < sys_sections.size(); i++) {
    
    // read interval time and create class for each at top defined status type
    sys.interval.push_back(configuration->readInterval(sys_sections[i]));
    sys.stat.push_back(new SystemStats(sys_sections[i], configFile));
    
    // if possible load from file
    sys.stat[i]->loadFromFile();
    
    syslog(LOG_DEBUG, "Main Thread: SystemStats class %s iniated.", sys_sections[i].c_str());
  }
  syslog(LOG_DEBUG, "Main Thread: All sys_stat objects created.");
		
  
  // the loop fires once every LOOP_TIME seconds
  int loopIteration = 0;
  while(loop) {
	
    // get the duration of function calling...
    time_t startTime = clock();
    
    
    
    // now do the actual system stat calculations
    for (int i = 0; i < sys_sections.size(); i++) {
      if ((sys.interval[i] != 0) && (loopIteration % sys.interval[i] == 0)) {
        sys.stat[i]->readStatus();
        syslog(LOG_DEBUG, "Main Thread: Triggered \"readStatus()\" in %s.", sys_sections[i].c_str());
      }
    }
    
	 
    // update counter
    if (loopIteration < MAX_TIME) { loopIteration++; } else { loopIteration = 0; }
			
    syslog(LOG_DEBUG, "Main Thread: loop no. %d finished", loopIteration);

    // now calculate how long we have to sleep
    time_t endTime = clock();
    int elapsedTime = (endTime - startTime)/CLOCKS_PER_SEC;
			
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
      printf("ServerStatus is currently not running. \n");
    }
  } else {
    printf("ServerStatus is currently not running. \n");
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