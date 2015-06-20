#include "csv.h"

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
CSV::CSV(string configFile, string section) {
  this->section = section;
	if (!loadConfigFile(configFile)) {
    syslog(LOG_ERR, "CSV %s: Config file [%s] could not be loaded.", section.c_str(), configFile.c_str());
  }
}

// destructor
CSV::~CSV() {
	// Something to be cleaned?
}



// load an existing csv file into data array
bool CSV::loadCSVfromFile(data output[], size_t array_size) {
  try {
    // check if csv file exists
    if (!file_exists(this->filepath + this->csv_filename)) {
      syslog(LOG_ERR, "CSV %s: Could not load existing CSV file [%s%s]. File does not exist.", this->section.c_str(), this->filepath.c_str(), this->csv_filename.c_str());
      return false;
    }
    
    ifstream file(this->filepath + this->csv_filename);
    std::vector<string> data_lines;
    string str; 
    if (delta) { output[0].value.clear(); }
    int lineNr = 0;
    while (getline(file, str))
    {
      lineNr++;
      if (lineNr == 1) { continue; }
      
      data_lines.push_back(str);
    }
    
    if ((data_lines.size() != this->sequence_length) || (data_lines.size() != array_size)) {
      syslog(LOG_WARNING, "CSV %s: Could not load existing CSV file [%s%s]. Datapoints mismatch configuration file.", this->section.c_str(), this->filepath.c_str(), this->csv_filename.c_str());
      return false;
    }
    
    
    for (int i = 0; i < data_lines.size(); i++) {
      vector<string> s = split(data_lines[i], ';');
      output[i].timestamp = s[0];
      output[i].value.clear();
      
      if (s.size() <= 1) { continue; }
      
      for (int j = 1; j < s.size(); j++) {
        s[j] = trim(s[j]);
        if (s[j].length() < 1) { continue; }
        output[i].value.push_back(atof(s[j].c_str()));
      }
    }
    
    syslog(LOG_DEBUG, "CSV %s: CSV file [%s%s] successfully loaded.", this->section.c_str(), this->filepath.c_str(), this->csv_filename.c_str());
    return true;
    
  } catch (int errorCode) {
    syslog(LOG_ERR, "CSV %s: Loading file [%s%s] failed. Errorcode: #%d", this->section.c_str(), this->filepath.c_str(), this->csv_filename.c_str(), errorCode);
    return false;
  }
}



// write data array to json file
bool CSV::writeCSVtoFile(data output[], size_t array_size, int position_pointer) {
  return false;
}







/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

bool CSV::loadConfigFile(string configFile){
  if (file_exists(configFile)) {
    
    config *configuration = new config(configFile);
    
    this->filepath = configuration->readFilepath();
    this->csv_filename = configuration->readCSVFilename(this->section);
    
    this->sequence_count = configuration->readSequenceCount(this->section);
    this->sequence_length = configuration->readElementCount(this->section);
    
    for (int i = 0; i < this->sequence_count; i++) {
      this->sequence_title.push_back(configuration->readSequenceTitle(this->section, i));
      this->sequence_color.push_back(configuration->readSequenceColor(this->section, i));
    }
    
    syslog(LOG_DEBUG, "CSV %s: All configuration loaded", this->section.c_str());
    return true;
  }

  return false;
}


// get next value of ring-array
void CSV::Inc(int &value, size_t array_size) {
  if (value < array_size - 1) { value++; }
  else { value = 0; }
}

// get previous value of ring-array
void CSV::Dec(int &value, size_t array_size) {
  if (value == 0) { value = array_size - 1; }
  else { value--; }
}