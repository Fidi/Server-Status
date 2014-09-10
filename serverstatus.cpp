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
#include "ini.h"

using namespace std;


#define MAX_TIME 120

#define PATHC 2
const string PATH[] = {"/usr/local/etc/serverstatus.conf", "/etc/serverstatus.conf"};




#define SYS_COUNT 6
const status SYS_TYPE[] = {CPU, Load, HDD, Mount, Memory, Network};
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


void flush_cin() {
  cin.clear();
  cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}


void configureOption(const string &configFile, const string section, const string description, const int defaultInterval) {
  // load ini-file:
  INI ini(configFile);

  system("clear");
  printf("============================================================= \n");
  printf("== Start the configuration for %s: \n", description.c_str());
  printf("============================================================= \n \n");

  flush_cin();
  char _char_input = 'Y';
  string _input;
  int _default_element;

  do {
    printf("Do you want to activate to read %s? [Y/n] ", description.c_str());
    scanf("%c", &_char_input);
  } while ((_char_input != 'Y') && (_char_input != 'y') && (_char_input != 'N') && (_char_input != 'n') && (_char_input != 10 ));
                  
  if ((_char_input == 'Y') || (_char_input == 'y') || (_char_input == 10)) { 

    int _numeric_value, _element_count;
    bool _valid = false;

    // read interval
    while( !_valid ) {
      printf("Enter the interval in which the %s shell be read (in minutes): [default %d] ", description.c_str(), defaultInterval);
      cin >> _input;
      
      if ((_numeric_value = atoi(_input.c_str())) == 0) {
        printf("You entered an invalid input. Please enter a numeric value greater than zero.\n");
      } else {
	    _valid = true;
        ini.writeInt(section, "interval", _numeric_value);
      }
      flush_cin();
    }

    // delta?
    do {
      printf("Do you want use delta values (print changes instead of absolute values)? [y/N] ");
      scanf("%c", &_char_input);
    } while ((_char_input != 'Y') && (_char_input != 'y') && (_char_input != 'N') && (_char_input != 'n') && (_char_input != 10 ));

    if ((_char_input == 'N') || (_char_input == 'n') || (_char_input == 10)) { 
      ini.writeInt(section, "delta", 0);
    } else {
      ini.writeInt(section, "delta", 1);
    }


    _valid = false;
    // read graph type     
    while( !_valid ) {
      printf("Which kind of graph type do you want to use? [line | bar | none] (e.g. used by iOS StatusBoard) ");
      cin >> _input;
      
      if ((_input != "line") && (_input != "bar") && (_input != "none")) {
        printf("You entered an invalid input. Please write \"line\", \"bar\" or \"none\".\n");
      } else {
	    _valid = true;
        ini.writeString(section, "graphtype", _input);
        if (_input == "bar") { _default_element = 1; }
        else { _default_element = 30; }
      }
      flush_cin();
    }


    _valid = false;
    // read array size         
    while( !_valid ) {
      printf("Enter how many old values should be stored: [default %d] ", _default_element);
      cin >> _input;
      
      if ((_numeric_value = atoi(_input.c_str())) == 0) {
        printf("You entered an invalid input. Please enter a numeric value greater than zero.\n");
      } else {
	    _valid = true;
        ini.writeInt(section, "elements", _numeric_value);
      }
      flush_cin();
    }


    _valid = false;
    // read element count         
    while( !_valid ) {
      printf("Enter how many different values exist (e.g. 2 cores, 5 discs, ...): ");
      cin >> _input;
      
      if ((_element_count = atoi(_input.c_str())) == 0) {
        printf("You entered an invalid input. Please enter a numeric value greater than zero.\n");
      } else {
	    _valid = true;
        ini.writeInt(section, "count", _element_count);
      }
      flush_cin();
    }

    // read description and command for each element
    for (int i = 0; i < _element_count; i++) {
      
        printf("Enter a description for element no. %d: ", i+1);
        getline(cin, _input);
        //flush_cin();
        ini.writeString(section, "desc" + IntToStr(i+1), _input);

        printf("Enter a shell command that returns a integer or float value for element no. %d: ", i+1);
        getline(cin, _input);
        //flush_cin();
        ini.writeString(section, "cmd" + IntToStr(i+1), _input);
    }

  } else {
    // setting the count to zero deactivates the function
    ini.writeInt(section, "count", 0);
    flush_cin();
  }

  printf("\n%s configuration successful. \nPress return to exit. \n", description.c_str());
  scanf("%c", &_char_input);
  
  // save changed ini file
  ini.saveToFile(configFile);
}




void runConfiguration(const string &configFile) {
  bool _exit = false;
  while (!_exit) {
    system("clear");
    printf("\n Please enter a number to start the configuration: \n");
    printf("  1) CPU temperature \n");
    printf("  2) Load Average \n");
    printf("  3) HDD temperature \n");
    printf("  4) Disc space \n");
    printf("  5) Memory \n");
    printf("  6) Network traffic \n");
    printf("  7) Exit \n \n");
    char _input;
    scanf("%c", &_input);

    if (_input != '7') {
      // configure selected option
      switch (_input) {
        case '1': configureOption(configFile, "CPU", "CPU temperature", 5); break;
        case '2': configureOption(configFile, "Load", "Load Average", 1); break;
        case '3': configureOption(configFile, "HDD", "HDD temperature", 60); break;
        case '4': configureOption(configFile, "Mount", "disc space", 60); break;
        case '5': configureOption(configFile, "Memory", "memory state", 1); break;
        case '6': configureOption(configFile, "Network", "network traffic", 1); break;
        default:  break;
      }
    } else { _exit = true; }
  }
}



void startDaemon(const string &configFile) {
  // check of an instance of serverstatus is already running in the background
  string cmd = "pgrep serverstatus";
  string did = getCmdOutput(&cmd[0]);
  if ((did != "") && (atoi(did.c_str()) != getpid())) {
    printf("Daemon is running already. \n");
    exit(EXIT_FAILURE);
  }


  pid_t pid, sid;
  pid = fork();

  // could not create child process
  if (pid < 0) { printf("Starting ServerStatus failed. \n"); exit(EXIT_FAILURE); }

  // child process created: terminate parent
  if (pid > 0) { printf("Starting ServerStatus... \n"); exit(EXIT_SUCCESS); }

  umask(0);

  // using syslog local1 for this daemon
  //setlogmask(LOG_UPTO (LOG_NOTICE));
  openlog("ServerStatus", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  syslog(LOG_NOTICE, "Started by User %d", getuid());

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


  INI ini(configFile);
  syslog(LOG_DEBUG, "Configuration file loaded.");

  sys_stat sys;
  for (int j = 0; j < SYS_COUNT; j++) {
    // read interval time and create class for each at top defined status type
    sys.interval.push_back(ini.readInt(getSectionFromType(SYS_TYPE[j]), "interval"));
    sys.stat.push_back(new SystemStats(SYS_TYPE[j], configFile));
  }     
  syslog(LOG_DEBUG, "Class objects created.");


		
  // the loop fires once every minute
  while(1) {
	
    // get the duration of function calling...
    startTime = clock();

    // for each status type read status if interval time is reached
    for (int j = 0; j < SYS_COUNT; j++) {
      if (i % sys.interval[j] == 0) { sys.stat[j]->readStatus(); }
    }
	
    // update counter
    if (i < MAX_TIME) { i++; } else { i = 0; }
			
    syslog (LOG_DEBUG, "loop no. %d finished", i);
			
    // now calculate how long we have to sleep
    endTime = clock();
    elapsedTime = (endTime - startTime)/CLOCKS_PER_SEC;
			
    sleep(60 - elapsedTime);  // let each loop last one minute
  }
   
  closelog();
}


void stopDaemon() {
  string cmd = "pgrep serverstatus";
  vector<string> output = split(getCmdOutput(&cmd[0]), 10);
  for (int i = 0; i < output.size(); i++) {
    if  (atoi(output[i].c_str()) != getpid()) {
      string killcmd = "kill " + output[i];
      system(killcmd.c_str());
    }
  }
  printf("ServerStatus stopped. \n");
}



/**********************************************************************
**** MAIN FUNCTION
**********************************************************************/
int main(int argc, char *argv[]) {
  
  // Load configuration
  string _configpath;
  if (!getConfigFilePath(_configpath)) {
    printf("Could not find a configuration file. \nMake sure a file called \"serverstatus.conf\" is located at \"/usr/local/etc/\" or \"/etc/\". \n");
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
  ***      "serverstatus --config" or "serverstatus -c"
  ***      "serverstatus --help" or "serverstatus -h"
  ***      "start", "restart" and "stop" 
  ***************************************************************/ 
  
  if ((argc == 0) || ((argc > 0) && (strcmp(argv[1],"start") == 0))) {
    // MODE 1: run the daemon
    startDaemon(_configpath);

  } else if (argc > 0) {
    // MODE 2: parse the options:
  
    if ((strcmp(argv[1], "--config") == 0) || (strcmp(argv[1], "-c") == 0)) {
      // Configuration mode:
      runConfiguration(_configpath);
      exit(EXIT_SUCCESS);
    }

    else if ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) {
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
  }
}