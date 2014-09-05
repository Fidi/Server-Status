#include <iostream>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "hdd.h"
#include "unix_functions.h"
#include "ini.h"

using namespace std;

/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

// default constructor
HDD::HDD() {
}

// constructor with defined element count
HDD::HDD(string configFile) {
  loadConfigFile(configFile);
  initArray();
}

// destructor
HDD::~HDD() {
  delete [] hdd_list;
}



// function to save the systems temperature into array
void HDD::readHDDTemperature() {
	
  // read cpu temp values:
  std::vector<double> newVal;
  for (int i=0; i<hdd_count; i++) {
    string disc_cmd = hdd_cmd[i];
    const char* disc_output = &getCmdOutput(&disc_cmd[0])[0];
    newVal.push_back(atof(disc_output));
  }

  setHDDTemperatureValue(getReadableTime(), newVal);

  // write json file:
  writeTemperatureJSONFile();

}




// function to save the load avarage into array
void HDD::readHDDUsage() {
	
  // get values:
  hdd_usage.mount.clear();
  for (int i=0; i<mount_count; i++) {
    string disc_cmd = mount_cmd[i];
    const char* disc_output = &getCmdOutput(&disc_cmd[0])[0];
    string disc_usage = disc_output;
    strReplace(disc_usage, "\n", "");
    hdd_usage.mount.push_back(disc_usage);
  }

  // write json file:
  writeUsageJSONFile(); 
}


/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

// loads the ini-based config and sets the variables
void HDD::loadConfigFile(string configFile) {

  INI ini(configFile);

  // read array sizes:
  hdd_elements = ini.readInt("HDD", "elements");
  mount_elements = ini.readInt("Mount", "elements");

  // read hdd infos
  hdd_count = ini.readInt("HDD", "count");
  for (int i=1; i<=hdd_count; i++) {
    hdd_desc.push_back(ini.readString("HDD", "desc" + IntToStr(i)));
    hdd_cmd.push_back(ini.readString("HDD", "cmd" + IntToStr(i)));
  }

  mount_count = ini.readInt("Mount", "count");
  for (int i=1; i<=mount_count; i++) {
    mount_desc.push_back(ini.readString("Mount", "desc" + IntToStr(i)));
    mount_cmd.push_back(ini.readString("Mount", "cmd" + IntToStr(i)));
  }

  file_path = ini.readString("General", "filepath");
}



// creates an array and fills it with empty values
void HDD::initArray() {
  // alloc memory for array
  hdd_list = new data_hdd[hdd_elements];
  //hdd_usage = new data_usage;
  
  // init array with empty values
  for (int i = 0; i < hdd_elements; i++) {
    hdd_list[i].timestamp = "0";

    for (int j = 0; j < hdd_count; j++) {
	  hdd_list[i].hdd_temp.push_back(0);
    }
  }

  for (int j = 0; j < mount_count; j++) {
    hdd_usage.mount.push_back("0");
  }

  hdd_position = 0;
}



// Setter for HDD Temperature
void HDD::setHDDTemperatureValue(std::string time, std::vector<double> value) {
  if (value.size() == hdd_count) {
	
    // save timestamp:
    hdd_list[hdd_position].timestamp = time;

    // get values:
    hdd_list[hdd_position].hdd_temp.clear();
    for (int i=0; i<hdd_count; i++) {
      hdd_list[hdd_position].hdd_temp.push_back(value[i]);
    }

    // move pointer:
    if (hdd_position < hdd_elements-1) {
      hdd_position++;
    } else {
      hdd_position = 0;
    }

  }
}



// generic function to export a variable number of hdd elements (array size) in json file
void HDD::writeTemperatureJSONFile() {

  ofstream tempFile;
  tempFile.open(file_path + "HDD_Temperature.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"HDD Temperature\", \n";
  tempFile << "  \"type\" : \"line\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 60, \n";
  tempFile << "  \"datasequences\" : [ \n";

  for (int j=0; j < hdd_count; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"" + hdd_desc[j] + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

	
    for (int i=hdd_position; i < hdd_elements; i++) {
      string val;
      stringstream out;
      out << hdd_list[i].hdd_temp[j];
      tempFile << "               { \"title\" : \"" + hdd_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (hdd_position != 0) {
      for (int i=0; i < hdd_position; i++) {
        string val;
        stringstream out;
        out << hdd_list[i].hdd_temp[j];
        tempFile << "               { \"title\" : \"" + hdd_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
      }
    }

    tempFile << "             ] \n";
    if (j == hdd_count-1) {
	  tempFile << "      } \n";
    } else {
      tempFile << "      }, \n";
    }

  }

  tempFile << "  ] \n";
  tempFile << "  } \n";
  tempFile << "} \n";
  tempFile.close();

}



// generic function to export usage with variable number of mounts
void HDD::writeUsageJSONFile() {

  ofstream tempFile;
  tempFile.open(file_path + "HDD_Usage.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"HDD Usage\", \n";
  tempFile << "  \"type\" : \"bar\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 300, \n";
  tempFile << "  \"datasequences\" : [ \n";
  tempFile << " { \n";
  tempFile << "        \"title\" : \"HDD Usage\", \n";
  tempFile << "        \"datapoints\" : [ \n";

  for (int j=0; j < mount_count; j++) {
	
    tempFile << "           { \"title\" : \"" + mount_desc[j] + "\", \"value\" : " + hdd_usage.mount[j] + "}, \n";
   
  }

  tempFile << "        ] \n";
  tempFile << "    } \n";

  tempFile << "  ] \n";
  tempFile << "  } \n";
  tempFile << "} \n";
  tempFile.close();

}

