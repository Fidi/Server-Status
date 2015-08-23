#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <syslog.h>
#include <cstring>
#include <algorithm>

#include "serverstatus.h"
#include "system_stats.h"
#include "unix_functions.h"
#include "config.h"
#include "communication.h"
#include "status_types.h"

#if __JSON__
  #include "json.h"
#endif
#if __CSV__
  #include "csv.h"
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
  if (loadConfigFile(this->configFile)) {
    initArray();
  } else {
    syslog(LOG_ERR, "SysStat %s: Failed to load config file [%s].", this->section.c_str(), this->configFile.c_str());
  }
  
  #if __JSON__
    if (this->output == OUT_JSON) {
      json_class = new JSON(configFile, this->section);
      syslog(LOG_NOTICE, "SysStats %s: JSON class created", this->section.c_str());
    }
  #endif
  
  #if __CSV__
    if (this->output == OUT_CSV) {
      csv_class = new CSV(configFile, this->section);
      syslog(LOG_NOTICE, "SysStats %s: CSV class created", this->section.c_str());
    }
  #endif
}

// destructor: clear up everything allocated
SystemStats::~SystemStats() {
  delete [] this->list;
  
  #if __JSON__
    delete this->json_class;
  #endif
  #if __CSV__
    delete this->csv_class;
  #endif
}



// collect data from the input source specified in the config file
bool SystemStats::readStatus() {
  std::vector<double> newVal;
  switch (this->input) {
  	case IN_CMD:    { // run command and use output:
                      for (int i = 0; i < this->element_count; i++) {
                        string cmd = this->cmd[i];
                        const char* cmd_output = &getCmdOutput(&cmd[0])[0];
                        newVal.push_back(atof(cmd_output));
                      }
                      break;
                    }
    case IN_SOCKET: { // read data from socket:
                      thread_value t = readValueGlobal(this->section, this->input_details.id);
                      for (int i = 0; i < t.value.size(); i++) {
                        newVal.push_back(t.value[i]);
                      }
                      break;
                    } 
    default:        { // no valid input was specified
                      syslog(LOG_WARNING, "SysStats %s: Could not read input.", this->section.c_str());
                      break;
                    }
  }
  
  if (newVal.size() > 0) {
    // add value to array
    if (!setValue(getReadableTime(), newVal)) { return false; }
    // and save array
    saveData();
  }
  
  return true;
}


// this will load the contents from a json file into the array:
bool SystemStats::loadFromFile(){
  bool result = false;
  switch (this->output) {
    case OUT_JSON:    { // submit data to json class that handles everything from here on
                        #if __JSON__
                          if (this->json_class != nullptr) {
                            result = this->json_class->loadJSONfromFile(this->list, this->array_size);
                          }
                        #endif
                        break;
                      }
    case OUT_CSV:     { // submit data to json class that handles everything from here on
                        #if __CSV__
                          if (this->csv_class != nullptr) {
                            result = this->csv_class->loadCSVfromFile(this->list, this->array_size);
                          }
                        #endif
                        break;
                      }
    default:          { // some unknown type of output was specified
                        syslog(LOG_WARNING, "SysStats %s: Could not load from file.", this->section.c_str());
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

// loads the cfg file and sets the variables
bool SystemStats::loadConfigFile(string configFile) {
	
  if (file_exists(configFile)) {
    
	  //this->section = getStringFromType(this->type);
    config *configuration = new config(configFile);
    
    // get input and output 
    this->input = getInputFromString(configuration->readInput(this->section));
    this->output = getOutputFromString(configuration->readOutput(this->section));
    
    // read array sizes (aka length of a sequence)
    this->array_size = configuration->readElementCount(this->section);
    if (configuration->readDelta(this->section)) {
      // right now only JSON supports delta
      if (this->output == OUT_JSON) {
        this->array_size++;
      }
    }
    
    // read number of sequences
    this->element_count = configuration->readSequenceCount(this->section);
    
    // get input commands
    for (int i = 0; i < this->element_count; i++) {
      this->cmd.push_back(configuration->readSequenceCommand(this->section, i));
    }
    
    // fill input/output sockets
    int port = configuration->readServerPort();
    this->input_details.port = port;
    this->output_details.port = port;
    bool ssl = configuration->readSSL();
    this->input_details.ssl = ssl;
    this->output_details.ssl = ssl;
    
    delete configuration;
    syslog(LOG_DEBUG, "SysStats %s: All configuration loaded", this->section.c_str());
    
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




// add collected data to ring array
bool SystemStats::setValue(std::string time, std::vector<double> value) {
	
  if (value.size() == this->element_count) {
    // save timestamp:
    this->list[this->list_position].timestamp = time;

    // get values:
    this->list[this->list_position].value.clear();
    for (int i=0; i<this->element_count; i++) {
      this->list[this->list_position].value.push_back(value[i]);
    }

    // move pointer:
    Inc(this->list_position);
    return true;
  } else {
    syslog(LOG_ERR, "SysStats %s: Could not add array entry. Sizes do not match.", this->section.c_str());
    return false;
  }
}


// use the collected data in the way the config file specifies
void SystemStats::saveData() {
  switch (this->output) {
    case OUT_JSON:    { // submit data to json class that handles everything from here on
                        #if __JSON__
                          if (this->json_class != nullptr) {
                            this->json_class->writeJSONtoFile(this->list, this->array_size, this->list_position);
                          } else {
                            syslog(LOG_ERR, "SysStats %s: failed to write to JSON file. Class not initiated.", this->section.c_str());
                          }
                        #endif
                        break;
                      }
    case OUT_CSV:     { // submit data to json class that handles everything from here on
                        #if __CSV__
                          if (this->csv_class != nullptr) {
                            this->csv_class->writeCSVtoFile(this->list, this->array_size, this->list_position);
                          } else {
                            syslog(LOG_ERR, "SysStats %s: failed to write to CSV file. Class not initiated.", this->section.c_str());
                          }
                        #endif
                        break;
                      }
    case OUT_SOCKET:  { // submit data to socket connection:
                        string msg = this->section + ", " + this->output_details.id;
                        int prev_position = this->list_position;
                        Dec(prev_position);
                        for (int i = 0; i < this->list[prev_position].value.size(); i++) {
                          msg = msg + ", " + to_string(this->list[prev_position].value[i]);
                        }
                        try {
                          connection c = create_socket(CLIENT, this->output_details.port, this->output_details.ip_address, this->output_details.ssl);
                          write_to_socket(c, msg);
                          destroy_socket(c);
                        } catch (int errorCode) {
                          syslog(LOG_ERR, "SysStats %s: Connection to socket failed [#%d].", this->section.c_str(), errorCode);
                        }                  
                        break;
                      }
    default: break;
  }
}



bool SystemStats::isReceiving(vector<string> input, string &sender_ip, string &clientID) {
  // first do some syntax checking
  if (input.size() != 5) {
    return false;
  } else {
    if ((input[0] != "RECEIVE") || (input[1] != "FROM") || (input[3] != "ID")) {
      return false;
    } else {
      // distribution has correct syntax
      sender_ip = input[2];
      clientID = input[4];
      return true;
    }
  }
}

bool SystemStats::isSending(vector<string> output, string &receiver_ip, string &clientID){
  // first do some syntax checking
  if (output.size() != 5) {
    return false;
  } else {
    if ((output[0] != "SEND") || (output[1] != "TO") || (output[3] != "ID")) {
      return false;
    } else {
      // distribution has correct syntax
      receiver_ip = output[2];
      clientID = output[4];
      return true;
    }
  } 
}


// get next value of ring-array
void SystemStats::Inc(int &value) {
  if (value < this->array_size - 1) { value++; }
  else { value = 0; }
}

// get previous value of ring-array
void SystemStats::Dec(int &value) {
  if (value == 0) { value = this->array_size - 1; }
  else { value--; }
}



// parse string to get enum input type
data_input SystemStats::getInputFromString(string input) {
  string s = input;
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  vector<string> str = split(s, ' ');
  
  // first check one word strings
  if (str[0] == "CMD") {
    return IN_CMD;
  } else  if (str[0] == "RECEIVE") {
    string ip, id;
    if (isReceiving(str, ip, id)) {
      input_details.ip_address = ip;
      input_details.id = id;
      return IN_SOCKET;
    }
    return IN_NONE;
  } else {
    return IN_NONE;
  }
}

// parse string to get enum output type
data_output SystemStats::getOutputFromString(string output) {
  string s = output;
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  vector<string> str = split(s, ' ');
  
  // first check one word strings
  if (str[0] == "JSON") {
    return OUT_JSON;
  } else  if (str[0] == "CSV") {
    return OUT_CSV;
  } else  if (str[0] == "SEND") {
    string ip, id;
    if (isSending(str, ip, id)) {
      output_details.ip_address = ip;
      output_details.id = id;
      return OUT_SOCKET;
    }
    return OUT_NONE;
  } else  if (str[0] == "POST") {
    return OUT_POST;
  } else {
    return OUT_NONE;
  }
}