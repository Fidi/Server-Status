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
#include "communication.h"


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
    case NOTIFY_HTTP_POST:  { // send the data to a webserver via HTTP_POST
                              try {
                                connection c = create_socket(CLIENT, this->http_port, this->http_host, false);
                                
                                if (this->http_identifier[strlen(this->http_identifier.c_str())-1] != '=') {
                                  this->http_identifier += "=";
                                }
                                
                                string post_msg = this->http_identifier;
                                for (int i = 0; i < output.value.size(); i++) {
                                  post_msg += to_string(output.value[i]) + ";";
                                }
                                
                                char sendline[200];
                                sprintf(sendline, 
                                  "POST %s HTTP/1.0\r\n"
                                  "Host: %s\r\n"
                                  "Content-type: application/x-www-form-urlencoded\r\n"
                                  "Content-length: %d\r\n\r\n"
                                  "%s\r\n", this->http_page.c_str(), this->http_host.c_str(), (unsigned int)strlen(post_msg.c_str()), post_msg.c_str());

                                write_to_socket(c, sendline);
                              
                                destroy_socket(c);
                                
                                result = true;
                              } catch (int error) {
                                syslog(LOG_ERR, "NOTIFY %s: Could not connect to server via HTTP_POST [%d].", this->section.c_str(), error);
                                result = false;
                              }
                              break;
                            }
    case NOTIFY_HTTP_GET:   { // send the data to a webserver via HTTP_GET
                              try {
                                connection c = create_socket(CLIENT, this->http_port, this->http_host, false);
                                
                                if (this->http_identifier[strlen(this->http_identifier.c_str())-1] != '=') {
                                  this->http_identifier += "=";
                                }
                                
                                string get_msg = this->http_identifier;
                                for (int i = 0; i < output.value.size(); i++) {
                                  get_msg += to_string(output.value[i]) + ";";
                                }
                                
                                string page_msg = this->http_page + "?" + get_msg;
                                char sendline[200];
                                sprintf(sendline, 
                                  "GET %s HTTP/1.0\r\n"
                                  "Host: %s\r\n"
                                  "Content-type: application/x-www-form-urlencoded\r\n"
                                  "Content-length: %d\r\n\r\n"
                                  "%s\r\n", page_msg.c_str(), this->http_host.c_str(), (unsigned int)strlen(get_msg.c_str()), get_msg.c_str());

                                write_to_socket(c, sendline);
                              
                                destroy_socket(c);
                                
                                result = true;
                              } catch (int error) {
                                syslog(LOG_ERR, "NOTIFY %s: Could not connect to server via HTTP_GET [%d].", this->section.c_str(), error);
                                result = false;
                              }
                              break;
                            }
    case NOTIFY_OS_X:       {  // send an push notification on a Mac running OS X 10.8 and higher
                              #ifdef __APPLE__
                                string msg = "osascript -e \'display notification \"";
                                for (int i = 0; i < output.value.size(); i++) {
                                  msg = msg + this->sequence_title[i] + " " + to_string(output.value[i]);
                                  if (i < output.value.size() - 1) { msg = msg + ", "; }
                                }
                                msg = msg + "\" with title \"ServerStatus\" subtitle \"" + this->notify_title + "\"\'";
                                getCmdOutput(&msg[0]);
                                result = true;
                              #else
                                syslog(LOG_ERR, "NOTIFY %s: Push notification only works with Mac OS X 10.8 and above.", this->section.c_str());
                                result = false;
                              #endif
                              break;
                            } 
    default:                {
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
    
    if ((this->notification_type == NOTIFY_HTTP_POST) || (this->notification_type == NOTIFY_HTTP_GET)) {
      this->http_host = configuration->readNotificationHttpHost(this->section);
      this->http_port = configuration->readNotificationHttpPort(this->section);
      this->http_page = configuration->readNotificationHttpPage(this->section);
      this->http_identifier = configuration->readNotificationHttpIdentifier(this->section);
    }
    
    delete configuration;
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