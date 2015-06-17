#include "json.h"

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
JSON::JSON(string configFile, string section) {
  this->section = section;
	if (!loadConfigFile(configFile)) {
    syslog(LOG_ERR, "JSON %s: Config file [%s] could not be loaded.", section.c_str(), configFile.c_str());
  }
}

// destructor
JSON::~JSON() {
	// Something to be cleaned?
}



// load an existing json file into data array
bool JSON::loadJSONfromFile(data output[], size_t array_size) {
  try {
    // check if json file exists
    if (!file_exists(this->filepath + this->json_filename)) {
      syslog(LOG_ERR, "JSON %s: Could not load existing JSON file [%s%s]. File does not exist.", this->section.c_str(), this->filepath.c_str(), this->json_filename.c_str());
      return false;
    }
    
    // the function uses some regex to extract the data points:
    string data_pattern = "{ \"title\" : ";
    string abs_pattern  = "\"absolute\": \"";
    string val_pattern  = "\"value\" : ";
    
    // open JSON file and read matching lines into vector
    ifstream file(this->filepath + this->json_filename);
    std::vector<string> data_lines;
    string str; 
    if (delta) { output[0].value.clear(); }
    while (getline(file, str))
    {
      // first check if this is an entry of the json datapoint array
      if (str.find(data_pattern) != std::string::npos) {
        data_lines.push_back(str);
      } 
      
      // if this section uses delta include absolute value, too
      if ((this->delta) && (str.find(abs_pattern) != std::string::npos)) {
        str.erase(0, str.find(abs_pattern) + abs_pattern.size());  
        str = str.substr(0, str.find("\","));
        this->absolute_value.push_back(atof(str.c_str()));
        output[0].value.push_back(atof(str.c_str()));
      }
    }
    
    // Now do some error checking:
      
    // check if line count matches the configuration:
    int datasize = this->sequence_length * this->sequence_count;
    //if (this->delta) { datasize -= this->element_count; }
    if (data_lines.size() != datasize) {
      syslog(LOG_WARNING, "JSON %s: Could not load existing JSON file [%s%s]. Datapoints mismatch configuration file.", this->section.c_str(), this->filepath.c_str(), this->json_filename.c_str());
      return false;
    }
    
    // this will avoid a seg-fault in case of a json typing error.
    if (this->delta) {
      while (this->absolute_value.size() < this->sequence_count) {
        this->absolute_value.push_back(0);
      }
    }
    
    
    // extract data and add them to the array
    string tmp_value;
    std::vector<double> val;
    int arrayValue;
    for (int currentValue = 0; currentValue < this->sequence_length; currentValue++) {
      
      if (delta) {
        arrayValue = currentValue + 1;
      } else {
        arrayValue = currentValue;
      }
      
      if (arrayValue >= array_size) { continue; }
      
      // 1) get timestamp
      tmp_value = data_lines[currentValue];
      tmp_value.erase(0, tmp_value.find(data_pattern) + data_pattern.size());
      output[arrayValue].timestamp = tmp_value.substr(tmp_value.find("\"")+1, tmp_value.find("\"", 2)-1);
      
      // 2) get values
      output[arrayValue].value.clear();
      for (int currentSequence = 0; currentSequence < this->sequence_count; currentSequence++) {
        tmp_value = data_lines[currentValue + (currentSequence*5)];
        tmp_value.erase(0, tmp_value.find(val_pattern) + val_pattern.size());  
        tmp_value = tmp_value.substr(0, tmp_value.find("}"));
        
        if (delta) {
          this->absolute_value[currentSequence] += atof(tmp_value.c_str());
          output[arrayValue].value.push_back(this->absolute_value[currentSequence]);
        } else {
          output[arrayValue].value.push_back(atof(tmp_value.c_str()));
        }
      }
    }
    
    // everything should have worked perfectly
    syslog(LOG_DEBUG, "JSON %s: JSON file [%s%s] successfully loaded.", this->section.c_str(), this->filepath.c_str(), this->json_filename.c_str());
    return true;
    
  } catch (int errorCode) {
    syslog(LOG_ERR, "JSON %s: Loading file [%s%s] failed. Errorcode: #%d", this->section.c_str(), this->filepath.c_str(), this->json_filename.c_str(), errorCode);
    return false;
  }
}



// write data array to json file
bool JSON::writeJSONtoFile(data output[], size_t array_size, int position_pointer) {
  try {
    
    ofstream out;
    out.open(this->filepath + this->json_filename);
    
    out << "{ \n";
    
    // write JSON header
    out << "  \"graph\" : { \n";
    out << "  \"title\" : \"" + this->json_title + "\", \n"; 
    out << "  \"type\" : \"" + getGraphTypeString(this->graph_type) + "\", \n";
    out << "  \"refreshEveryNSeconds\" : " << this->refresh_interval << ", \n";
     
    // write JSON sequences 
    out << "  \"datasequences\" : [ \n";

    for (int j = 0; j < this->sequence_count; j++) {
    	
      // write sequence header 
      out << "      {";
      out << "      \"title\": \"" << this->sequence_title[j] << + "\", \n";
      if (this->sequence_color[j] != "-") {
        out << "             \"color\": \"" << this->sequence_color[j] << + "\", \n";
      }
      if (this->delta) {
        out << "             \"absolute\": \"" << output[position_pointer].value[j] << + "\", \n";
      }
        

      int datapoint_count = array_size;
      int current_position = position_pointer;
      if (this->delta) { 
        Inc(current_position, array_size);
        datapoint_count--;
      }
        
      // write data points
      out << "             \"datapoints\" : [ \n";
      
      for (int i = 0; i < datapoint_count; i++) {
        
        // avoid seg-fault
        if (current_position >= array_size) { continue; }
        
        double val = output[current_position].value[j];
        if (this->delta) {
          int previous_position = current_position;
          Dec(previous_position, array_size);
          val -= output[previous_position].value[j];
        }
        
        if (i == datapoint_count -1) {
          out << "               { \"title\" : \"" + output[current_position].timestamp + "\", \"value\" : " + to_string(val) + "} \n";
        } else {
          out << "               { \"title\" : \"" + output[current_position].timestamp + "\", \"value\" : " + to_string(val) + "}, \n";
        }
        
        Inc(current_position, array_size);
      }


      out << "             ] \n";
      if (j == this->sequence_count - 1) {
    	  out << "      } \n";
      } else {
        out << "      }, \n";
      }

    }
      
    out << "  ] \n";
    out << "  } \n";
    out << "} \n";
      
    out.close();
    
    syslog(LOG_DEBUG, "JSON %s: File [%s%s] successfully written.", this->section.c_str(), this->filepath.c_str(), this->json_filename.c_str());
    return true;
    
  } catch (int error) {
    syslog(LOG_ERR, "JSON %s: Error writing JSON file [%s%s]. Errorcode: #%d.", this->section.c_str(), this->filepath.c_str(), this->json_filename.c_str(), error);
    return false;
  }
}







/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

bool JSON::loadConfigFile(string configFile){
  
  if (file_exists(configFile)) {
    
    config *configuration = new config(configFile);
    
    this->filepath = configuration->readFilepath();
    this->json_filename = configuration->readJSONFilename(this->section);
    
    this->json_title = configuration->readJSONTitle(this->section);
    
    this->sequence_count = configuration->readSequenceCount(this->section);
    this->sequence_length = configuration->readElementCount(this->section);

    for (int i = 0; i < this->sequence_count; i++) {
      this->sequence_title.push_back(configuration->readSequenceTitle(this->section, i));
      this->sequence_color.push_back(configuration->readSequenceColor(this->section, i));
    }
    
    this->delta = configuration->readDelta(this->section);
    
    this->refresh_interval = configuration->readJSONRefreshInterval(this->section);
    
    this->graph_type = getGraphTypeFromString(configuration->readJSONType(this->section));
    
    syslog(LOG_DEBUG, "JSON %s: All configuration loaded", this->section.c_str());
    return true;
  }

  return false;
}



string JSON::getGraphTypeString(json_graph type) {
  string graphtype;
  switch (type) {
    case JSON_BAR: graphtype = "bar";  break;
    case JSON_PIE: graphtype = "pie";  break;
    default:       graphtype = "line"; break;
  } 
  return graphtype;
}

json_graph JSON::getGraphTypeFromString(string type) {
  if (type == "line") {
    return JSON_LINE;
  } else if (type == "bar") {
    return JSON_BAR;
  } else if (type == "pie") {
    return JSON_PIE;
  } else {
    return JSON_AUTO;
  }
}


// get next value of ring-array
void JSON::Inc(int &value, size_t array_size) {
  if (value < array_size - 1) { value++; }
  else { value = 0; }
}

// get previous value of ring-array
void JSON::Dec(int &value, size_t array_size) {
  if (value == 0) { value = array_size - 1; }
  else { value--; }
}