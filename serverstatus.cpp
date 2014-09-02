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

#include "cpu.h"
#include "hdd.h"
#include "unix_functions.h"

using namespace std;


#define MAX_TIME 120



int main(int argc, char *argv[]) {
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


  sid = setsid();
  if (sid < 0) {
    syslog (LOG_ERR, "ServerStatus: Could not create new sid for child process");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "ServerStatus: new SID for child process created.");


  if ((chdir("/")) < 0) {
    syslog (LOG_ERR, "ServerStatus: Could not change working directory");
    exit(EXIT_FAILURE);
  }
  syslog(LOG_DEBUG, "ServerStatus: Changed working directory to root directory.");


  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);


  // daemon is ready: now start the actual work
  syslog(LOG_NOTICE, "ServerStatus: started by User %d", getuid());

  
  // variables
  int i = 0;
  int elapsedTime = 0;
		
  time_t startTime,endTime;
		
  // create hdd / cpu objects
  HDD hdd_class(10);
  CPU cpu_class(10);

		
  // the main loop: here comes all the stuff that has to be repeated 
  // the loop fires once every minute
  while(1) {
	
    // get the duration of function calling...
    startTime = clock();

    if (i % 60 == 0) {
      // scripts that are executed once every hour:
      hdd_class.readHDDInfos();
    }

    // scripts that are executed every minute
    cpu_class.readCPUInfos();
			
    // update counter
    if (i < MAX_TIME) { i++; }
    else { i = 0; }
			
    syslog (LOG_DEBUG, "ServerStatus: loop no. %d finished", i);
			
    // now calculate how long we have to sleep
    endTime = clock();
    elapsedTime = (endTime - startTime)/CLOCKS_PER_SEC;
			
    sleep(60 - elapsedTime);  // let each loop last one minute
  }
   
  closelog();
  exit(EXIT_SUCCESS);	
}