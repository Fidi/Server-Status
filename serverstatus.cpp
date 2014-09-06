/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Kevin Fiedler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

#include "system_stats.h"
#include "unix_functions.h"
#include "ini.h"

using namespace std;


#define MAX_TIME 120

#define PATHC 2
const string PATH[] = {"/usr/local/etc/serverstatus.conf", "/etc/serverstatus.conf"};


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
  ***     Right now that only enables configuration mode.
  ***     Open "serverstatus --config" or "serverstatus -c"
  ***************************************************************/ 
  
  
 
  if ((argc > 0) && (strcmp(argv[1],"start") != 0)) {
    // MODE 2:

    // Change into config mode:
    if ((strcmp(argv[1], "--config") == 0) || (strcmp(argv[1], "-c") == 0)) {
      // TODO!
      exit(EXIT_SUCCESS);
    }

    // kill any running instances of serverstatus
    if (strcmp(argv[1], "stop") == 0) {
      string cmd = "pgrep serverstatus";
      if (getCmdOutput(&cmd[0]) != "") {
        cmd = "killall serverstatus";
        getCmdOutput(&cmd[0]);
        printf("Process serverstatus stopped. \n");
        exit(EXIT_SUCCESS);
      }

    }

  } else {
    // MODE 1:
    // Start a daemon process and read the system statistics in the background


    // check of an instance of serverstatus is already running in the background
    string cmd = "pgrep serverstatus";
	if (getCmdOutput(&cmd[0]) != "") {
      printf("Daemon is running already. \n");
      exit(EXIT_FAILURE);
    }


    pid_t pid, sid;
  
    pid = fork();

    // could not create child process
    if (pid < 0) { exit(EXIT_FAILURE); }

    // child process created: terminate parent
    if (pid > 0) { exit(EXIT_SUCCESS); }

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


    INI ini(_configpath);

    int hdd_interval = ini.readInt("HDD", "interval");
    int mount_interval = ini.readInt("Mount", "interval");
    int cpu_interval = ini.readInt("CPU", "interval");
    int load_interval = ini.readInt("Load", "interval");
    int memory_interval = ini.readInt("Memory", "interval");
  
    syslog(LOG_DEBUG, "Configuration file loaded.");

		
    // create system stat classes
    SystemStats cpu(CPU, _configpath);
    SystemStats load(Load, _configpath);
    SystemStats hdd(HDD, _configpath);
    SystemStats mount(Mount, _configpath);
    SystemStats mem(Memory, _configpath);

    syslog(LOG_DEBUG, "Class objects created.");

		
    // the main loop: here comes all the stuff that has to be repeated 
    // the loop fires once every minute
    while(1) {
	
      // get the duration of function calling...
      startTime = clock();

      if (i % hdd_interval == 0) {
        hdd.readStatus();
      }

      if (i % mount_interval == 0) {
        mount.readStatus();
      }

      if (i % cpu_interval == 0) {
        cpu.readStatus();
      }

      if (i % load_interval == 0) {
        load.readStatus();
      }
 
      if (i % memory_interval == 0) {
        mem.readStatus();
      }
			
      // update counter
      if (i < MAX_TIME) { i++; }
      else { i = 0; }
			
      syslog (LOG_DEBUG, "loop no. %d finished", i);
			
      // now calculate how long we have to sleep
      endTime = clock();
      elapsedTime = (endTime - startTime)/CLOCKS_PER_SEC;
			
      sleep(60 - elapsedTime);  // let each loop last one minute
    }
   
    closelog();
    exit(EXIT_SUCCESS);	
  }
}