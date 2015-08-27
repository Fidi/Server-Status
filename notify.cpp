#include "notify.h"

#include <iostream>
#include <string>
#include <syslog.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <syslog.h>
#include <cstring>

#include "unix_functions.h"
#include "config.h"


using namespace std;

/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

// constructor: load config file into class
NOTIFY::NOTIFY(string configFile, string section) {
  this->section = section;
	if (!loadConfigFile(configFile)) {
    syslog(LOG_ERR, "NOTIFY %s: Config file [%s] could not be loaded.", section.c_str(), configFile.c_str());
  }
}

// destructor
NOTIFY::~NOTIFY() {
	// Something to be cleaned?
}


bool NOTIFY::notify(data output){
  bool result;
  switch (this->notification_type) {
    case NOTIFY_OS_X:     {
                            #ifdef __APPLE__
                              string msg = "osascript -e \'display notification \"";
                              for (int i = 0; i < output.value.size(); i++) {
                                msg = msg + this->sequence_title[i] + " " + to_string(output.value[i]);
                                if (i < output.value.size() - 1) { msg = msg + ", "; }
                              }
                              msg = msg + "\" with title \"ServerStatus\" subtitle \"" + this->notify_title + "\"\'";
                              syslog(LOG_WARNING, "%s", msg.c_str());
                              getCmdOutput(&msg[0]);
                              result = true;
                            #else
                              syslog(LOG_ERR, "NOTIFY %s: Push notification only works with Mac OS X 10.8 and above.", this->section.c_str());
                              result = false;
                            #endif
                            break;
                          } 
    default:              {
                            syslog(LOG_WARNING, "NOTIFY %s: No valid notification type selected.", this->section.c_str());
                            result = false;
                            break;
                          }    
  }
  return result;
}




/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

bool NOTIFY::loadConfigFile(string configFile){
  if (file_exists(configFile)) {
    
    config *configuration = new config(configFile);
    
    this->sequence_count = configuration->readSequenceCount(this->section);
    
    this->notify_title = configuration->readNotificationTitle(this->section);
    for (int i = 0; i < this->sequence_count; i++) {
      this->sequence_title.push_back(configuration->readSequenceTitle(this->section, i));
    }
    
    this->notification_type = this->getNotificationTypeFromString(configuration->readNotificationType(this->section));
    
    syslog(LOG_DEBUG, "CSV %s: All configuration loaded", this->section.c_str());
    return true;
  }

  return false;
}

notify_type NOTIFY::getNotificationTypeFromString(string input) {
  string s = input;
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  vector<string> str = split(s, ' ');
  
  // first check one word strings
  if (str[0] == "HTTP_POST") {
    return NOTIFY_HTTP_POST;
  } else  if (str[0] == "HTTP_GET") {
    return NOTIFY_HTTP_GET;
  } else  if (str[0] == "OS_X") {
    return NOTIFY_OS_X;
  } else if (str[0] == "SYSLOG") {
    return NOTIFY_SYSLOG;
  } else {
    return NOTIFY_NONE;
  }
}