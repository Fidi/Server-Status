#include "status_types.h"

#include <iostream>
#include <stdio.h>
#include <string>

using namespace std;

/*****************************************************************
**
**  CONVERSION FUNCTIONS
**
*****************************************************************/
string getStringFromType(status type) {
  string res;
  switch (type) {
    case CPU: res     = "cpu";     break;
    case Load: res    = "load";    break;
    case HDD: res     = "hdd";     break;
    case Mount: res   = "mount";   break;
    case Memory: res  = "memory";  break;
    case Network: res = "network"; break;
    default: throw "No status type submitted."; break;
  }
  
  return res;
}

status getTypeFromString(string type) {
  if (type == "cpu") { 
    return CPU; 
  } else if (type == "load") { 
    return Load; 
  } else if (type == "hdd") { 
    return HDD; 
  } else if (type == "mount") { 
    return Mount; 
  } else if (type == "memory") { 
    return Memory; 
  } else if (type == "network") { 
    return Network; 
  } else  { 
    throw "No valid status type found.";
    return None; 
  }
}