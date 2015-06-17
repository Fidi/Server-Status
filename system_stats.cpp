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
#include "status_types.h"

#if __JSON__
  #include "json.h"
#endif

using namespace std;


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

// constructor: load config and init class
SystemStats::SystemStats(string section, string configFile){
  // set the type (cpu, load, hdd, mount or memory):
  this->section = section;
  this->configFile = configFile;
  
  // now init the generic class with the config file
  if (loadConfigFile(configFile)) {
    if ( this->delta) { this->array_size++; }
    initArray();
  } 
  
  #if __JSON__
    if (this->output == OUT_JSON) {
      json_class = new JSON(configFile, this->section);
      syslog(LOG_NOTICE, "SysStats %s: JSON class created", this->section.c_str());
    }
  #endif
}

// destructor
SystemStats::~SystemStats() {
  delete [] this->list;
  
  #if __JSON__
    delete this->json_class;
  #endif
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
    saveData();
  }
}


// this will load the contents from a json file into the array:
bool SystemStats::loadFromFile(){
  /*
  try {
    
    // check for application mode and file existance 
    if (((this->_output_details.size() >= 1) && (this->_output_details[0] != "JSON")) || (!file_exists(this->filepath + this->section + ".json"))) {
      syslog(LOG_NOTICE, "Could not load existing json files.");
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
    
  } catch (int e) {
    return false;
  } 
  */
  return true;
}



/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

// loads the cfg file and sets the variables
bool SystemStats::loadConfigFile(string configFile) {
	
  if (file_exists(configFile)) {
    
	  //this->section = getStringFromType(this->type);
    config *configuration = new config(configFile);

    // read array sizes:
    this->array_size = configuration->readElementCount(this->section);

    // read number of informations:
    this->element_count = configuration->readSequenceCount(this->section);
    
    this->input = IN_CMD;
    this->output = OUT_JSON;
    
    
    
    for (int i=1; i<=this->element_count; i++) {
      this->cmd.push_back(configuration->readSequenceCommand(this->section, i-1));
      this->description.push_back(configuration->readSequenceTitle(this->section, i-1));
      this->color.push_back(configuration->readSequenceColor(this->section, i-1));
    }

    string _type = configuration->readJSONType(this->section);
    if (_type == "line") {
       this->json_type = JSON_LINE;
    } else if (_type == "bar") {
      this->json_type = JSON_BAR;
    } else if (_type == "pie") {
      this->json_type = JSON_PIE;
    } else {
      this->json_type = JSON_AUTO;
    }
    
    string s = configuration->readInput(this->section);
    this->_input_details = split(s, ' ');
    s = configuration->readOutput(this->section);
    this->_output_details = split(s, ' ');
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

void SystemStats::saveData() {
  switch (this->output) {
    case OUT_JSON:  { // submit data to json class that handles everything from here on
                      #if __JSON__
                        if (this->json_class != nullptr) {
                          this->json_class->writeJSONtoFile(this->list, this->array_size, this->list_position);
                        }
                      #endif
                      break;
                    }
    default: break;
  }
}



bool SystemStats::isReceiving(string &sender_ip, string &clientID) {
  // first do some syntax checking
  if (this->_input_details.size() != 5) {
    return false;
  } else {
    if ((this->_input_details[0] != "RECEIVE") || (this->_input_details[1] != "FROM") || (this->_input_details[3] != "ID")) {
      return false;
    } else {
      // distribution has correct syntax
      sender_ip = this->_input_details[2];
      clientID = this->_input_details[4];
      return true;
    }
  }
}

bool SystemStats::isSending(string &receiver_ip, string &clientID){
  // first do some syntax checking
  if (this->_output_details.size() != 5) {
    return false;
  } else {
    if ((this->_output_details[0] != "SEND") || (this->_output_details[1] != "TO") || (this->_output_details[3] != "ID")) {
      return false;
    } else {
      // distribution has correct syntax
      receiver_ip = this->_output_details[2];
      clientID = this->_output_details[4];
      return true;
    }
  } 
}