#ifndef _notify_hpp_
#define _notify_hpp_

#include <string.h>
#include "status_types.h"

// different ways of notification
enum _notify_type_t {
  NOTIFY_HTTP_POST,      // HTTP Protocol POST
  NOTIFY_HTTP_GET,       // HTTP Protocol GET
  NOTIFY_OS_X,           // OS X Notification 
  NOTIFY_SYSLOG,         // Syslog
  NOTIFY_NONE
};
typedef enum _notify_type_t notify_type;


class NOTIFY
{
  public:
    NOTIFY(std::string configFile, std::string section);
    ~NOTIFY();
    
    bool notify(data output);

  private:
    
    std::string section;
    
    notify_type notification_type;
    
    int sequence_count;

    std::string notify_title;
    std::vector<std::string> sequence_title;
    
    bool loadConfigFile(std::string configFile);
    
    notify_type getNotificationTypeFromString(std::string input);
};

#endif