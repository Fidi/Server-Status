#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <syslog.h>
#include <cstring>

#include "serverstatus.h"
#include "system_stats.h"
#include "unix_functions.h"
#include "config.h"
#include "communication.h"

using namespace std;


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

SystemStats::SystemStats(status type, string configFile){
  // set the type (cpu, load, hdd, mount or memory):
  this->type = type;
  
  // now init the generic class with the config file
  if (loadConfigFile(configFile)) {
    if ( this->delta) { this->array_size++; }
    initArray();
  } 
}

SystemStats::~SystemStats() {
  delete [] this->list;
}




// function to save the systems temperature into array
void SystemStats::readStatus() {
  // read requested values:
  std::vector<double> newVal;
  
  string ip;
  string id;
  
  // distinguish between server | client | standalone
  if (isReceiving(ip, id)) {
    // we are in server receive mode
    thread_value t = readValueGlobal(this->type, id);
    for (int i = 0; i < t.value.size(); i++) {
      newVal.push_back(t.value[i]);
    }
  } else {
    // we are in bash command mode
    for (int i = 0; i < this->element_count; i++) {
      string cmd = this->cmd[i];
      const char* cmd_output = &getCmdOutput(&cmd[0])[0];
      newVal.push_back(atof(cmd_output));
    }
  }
  
  // if client mode send string to server
  if (isSending(ip, id)) {
    string msg = getStringFromType(this->type) + ", " + id;
    for (int i = 0; i < newVal.size(); i++) {
      msg = msg + ", " + to_string(newVal[i]);
    }
    connection c = create_socket(CLIENT, this->port, ip, this->ssl);
    write_to_socket(c, msg);
    destroy_socket(c);
    newVal.clear();
  } 
  
  // save values into array and write to file
  if (newVal.size() > 0) {
    setValue(getReadableTime(), newVal);
    writeJSONFile();
  }
}


// this will load the contents from a json file into the array:
bool SystemStats::loadFromFile(){
  
  // check for application mode and file existance 
  if ((this->distribution[0] == "SEND") || (!file_exists(this->filepath + this->section + ".json"))) {
    syslog(LOG_DEBUG, "Could not load existing json files.");
    return false;
  }
  
  std::vector<string> lines;
  string pattern0 = "{ \"title\" : ";
  string pattern1 = "\"absolute\": \"";
  
  // find lines that have datapoints:
  ifstream file(this->filepath + this->section + ".json");
  string str; 
  while (getline(file, str))
  {
    if (str.find(pattern0) != std::string::npos) {
      lines.push_back(str);
    } else if ((this->delta) && (str.find(pattern1) != std::string::npos)) {
      str.erase(0, str.find(pattern1) + pattern1.size());  
      str = str.substr(0, str.find("\","));
      this->delta_abs_value.push_back(atof(str.c_str()));
    }
  }
  
  // now all data values are inside the lines-vector.
  // check if their size matches the configuration:
  int datasize = this->array_size * this->element_count; 
  if (this->delta) { datasize -= this->element_count; }
  if (lines.size() != datasize) {
    syslog(LOG_WARNING, "%s: Could not load existing json files. Datapoints mismatch configuration file.", this->section.c_str());
    return false;
  }
  
  // for delta add the previous value for correct calculation
  if (this->delta) {
    setValue("-", this->delta_abs_value);
  }
  
  // extract data and add them to the array
  string tmp;
  string tim;
  string va;
  string pattern2 = "\"value\" : ";
  std::vector<double> val;
  for (int j = 0; j < datasize/this->element_count; j++) {
    // 1) get time:
    tmp = lines[j];
    tmp.erase(0, tmp.find(pattern0) + pattern0.size());
    tim = tmp.substr(tmp.find("\"")+1, tmp.find("\"", 2)-1);
    
    // 2) get values:
    val.clear();
    for (int i = 0; i < this->element_count; i++) {
      tmp = lines[j + (i*(this->array_size-1))];
      tmp.erase(0, tmp.find(pattern2) + pattern2.size());  
      va = tmp.substr(0, tmp.find("}"));
      // if atof fails it will write "0"
      if (this->delta) {
        this->delta_abs_value[i] = this->delta_abs_value[i] + atof(va.c_str());
        val.push_back(this->delta_abs_value[i]);
      } else {
        val.push_back(atof(va.c_str()));
      }
    }
    
    // 3) add record
    setValue(tim, val);
  }

  return true;
}



/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

// loads the ini-based config and sets the variables
bool SystemStats::loadConfigFile(string configFile) {
	
  if (file_exists(configFile)) {
	  this->section = getStringFromType(this->type);
    config *configuration = new config(configFile);

    // read array sizes:
    this->array_size = configuration->readElementCount(this->section);

    // read number of informations:
    this->element_count = configuration->readSequenceCount(this->section);
    for (int i=1; i<=this->element_count; i++) {
      this->cmd.push_back(configuration->readSequenceCommand(this->section, i-1));
      this->description.push_back(configuration->readSequenceTitle(this->section, i-1));
      this->color.push_back(configuration->readSequenceColor(this->section, i-1));
    }

    string _type = configuration->readJSONType(this->section);
    if (_type == "bar") {
      this->json_type = bar;
    } else if (_type == "pie") {
      this->json_type = pie;
    } else {
      this->json_type = line;
    }
    
    string s = configuration->readDistribution(this->section);
    this->distribution = split(s, ' ');
    this->port = configuration->readServerPort();
    this->ssl = configuration->readSSL();
    
    this->delta = configuration->readDelta(this->section);
    
    this->refresh_interval = configuration->readJSONRefreshInterval(this->section);

    this->interval = configuration->readInterval(this->section);
    this->filepath = configuration->readFilepath();
    
    syslog(LOG_DEBUG, "All configuration loaded");
    
    return true;
  }

  return false;
}




// creates an array and fills it with empty values
void SystemStats::initArray() {

  // alloc memory for array
  this->list = new data[this->array_size];
  
  // init array with empty values
  for (int i = 0; i < this->array_size; i++) {
    this->list[i].timestamp = "0";

    for (int j = 0; j < this->element_count; j++) {
      this->list[i].value.push_back(0);
    }
  }

  // set pointer
  this->list_position = 0;
}




// Array setter:
void SystemStats::setValue(std::string time, std::vector<double> value) {
	
  if (value.size() == this->element_count) {
    
    // save timestamp:
    this->list[this->list_position].timestamp = time;

    // get values:
    this->list[this->list_position].value.clear();
    for (int i=0; i<this->element_count; i++) {
      this->list[this->list_position].value.push_back(value[i]);
    }

    // move pointer:
    if (this->list_position < this->array_size - 1) {
      this->list_position++;
    } else {
      this->list_position = 0;
    }

  }
}




bool SystemStats::isReceiving(string &sender_ip, string &clientID) {
  // first do some syntax checking
  if (this->distribution.size() != 5) {
    return false;
  } else {
    if ((this->distribution[0] != "RECEIVE") || (this->distribution[1] != "FROM") || (this->distribution[3] != "ID")) {
      return false;
    } else {
      // distribution has correct syntax
      sender_ip = this->distribution[2];
      clientID = this->distribution[4];
      return true;
    }
  }
}

bool SystemStats::isSending(string &receiver_ip, string &clientID){
  // first do some syntax checking
  if (this->distribution.size() != 5) {
    return false;
  } else {
    if ((this->distribution[0] != "SEND") || (this->distribution[1] != "TO") || (this->distribution[3] != "ID")) {
      return false;
    } else {
      // distribution has correct syntax
      receiver_ip = this->distribution[2];
      clientID = this->distribution[4];
      return true;
    }
  } 
}




void SystemStats::Inc(int &value) {
  if (value < this->array_size) { value++; }
  else { value = 0; }
}



// generic json export function
void SystemStats::writeJSONFile() {
  ofstream out;
  out.open(this->filepath + this->section + ".json");
  out << "{ \n";
  out << "  \"graph\" : { \n";
  out << "  \"title\" : \"" + this->section + "\", \n";
  
  string graphtype;
  switch (this->json_type) {
    case bar: graphtype = "bar";  break;
    case pie: graphtype = "pie";  break;
    default:  graphtype = "line"; break;
  }  
  out << "  \"type\" : \"" + graphtype + "\", \n";
  out << "  \"refreshEveryNSeconds\" : " << this->refresh_interval << ", \n";
  
  out << "  \"datasequences\" : [ \n";
  
  // for every load value print a line:
  for (int j = 0; j < this->element_count; j++) {
	
    out << "      {";
    out << "      \"title\": \"" << this->description[j] << + "\", \n";
    if (this->color[j] != "-") {
      out << "             \"color\": \"" << this->color[j] << + "\", \n";
    }
    
    
    double val;
    int start_position = this->list_position;
    if (this->delta) { Inc(start_position); }
    
    if (this->delta) {
      out << "             \"absolute\": \"" << this->list[this->list_position].value[j] << + "\", \n";
    }
    
    out << "             \"datapoints\" : [ \n";


    for (int i = start_position; i < this->array_size; i++) {
      val = this->list[i].value[j];
      if (this->delta) { if (i == 0) { val -= this->list[this->array_size-1].value[j]; } else { val -= this->list[i-1].value[j]; } }
      if ((this->list_position == 0) && (i == this->array_size-1)) {
        out << "               { \"title\" : \"" + this->list[i].timestamp + "\", \"value\" : " + to_string(val) + "} \n";
      } else {
        out << "               { \"title\" : \"" + this->list[i].timestamp + "\", \"value\" : " + to_string(val) + "}, \n";
      }
      
    }

    if (this->list_position != 0) {
      // print the remaining elements
      for (int i=0; i < this->list_position; i++) {
        val = this->list[i].value[j];
        if (this->delta) { if (i == 0) { val -= this->list[this->array_size-1].value[j]; } else { val -= this->list[i-1].value[j]; } }
        if (i == this->list_position - 1) {
          out << "               { \"title\" : \"" + this->list[i].timestamp + "\", \"value\" : " + to_string(val) + "} \n";
        } else {
          out << "               { \"title\" : \"" + this->list[i].timestamp + "\", \"value\" : " + to_string(val) + "}, \n";
        }
        
      }
    }

    out << "             ] \n";
    if (j == this->element_count - 1) {
	  out << "      } \n";
    } else {
      out << "      }, \n";
    }

  }
  
  out << "  ] \n";
  out << "  } \n";
  out << "} \n";
  
  out.close();
}




/*****************************************************************
**
**  HELPER FUNCTIONS
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