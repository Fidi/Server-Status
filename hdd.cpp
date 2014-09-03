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



template <typename T, int size>
int getArraySize(T(&) [size]) {
    return size;
}


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

// default constructor
HDD::HDD() {
}

// constructor with defined element count
HDD::HDD(std::string configFile) {
  loadConfigFile(configFile);
  initArray();
}

// destructor
HDD::~HDD() {
  delete [] hdd_list;
}


/*
void HDD::readHDDInfos() {
  // get current time (UNIX format)
  hdd_list[array_position].timestamp = getReadableTime();

  readCurrentTemperature();
  readCurrentUsage();
	
  // move to next array element
  if (array_position < hdd_elements-1) {
    array_position++;
  } else {
    array_position = 0;
  }

  writeTemperatureJSONFile();
  writeUsageJSONFile();
}
*/



// function to save the systems temperature into array
void HDD::readHDDTemperature() {
		  
  // save timestamp:
  hdd_list[hdd_position].timestamp = getReadableTime();

  // get values:
  hdd_list[hdd_position].hdd_temp.clear();
  for (int i=0; i<hdd_count; i++) {
    std::string disc_cmd = hdd_cmd[i];
    const char* disc_output = &getCmdOutput(&disc_cmd[0])[0];
    hdd_list[hdd_position].hdd_temp.push_back(atof(disc_output));
  }

  // move pointer:
  if (hdd_position < hdd_elements-1) {
    hdd_position++;
  } else {
    hdd_position = 0;
  }

  // write json file:
  writeTemperatureJSONFile();

}




// function to save the load avarage into array
void HDD::readHDDUsage() {
	
  // get values:
  hdd_usage.mount.clear();
  for (int i=0; i<mount_count; i++) {
    std::string disc_cmd = mount_cmd[i];
    const char* disc_output = &getCmdOutput(&disc_cmd[0])[0];
    std::string disc_usage = disc_output;
    strReplace(disc_usage, "\n", "");
    hdd_usage.mount.push_back(disc_usage);
  }

  // write json file:
  writeUsageJSONFile();


	/*
  // free space
  std::string cmd = "df | grep ^Data/Filme | awk '{print $4}'";
  const char* output = &getCmdOutput(&cmd[0])[0];
  std::string fixedOutput = output;
  strReplace(fixedOutput, "\n", "");
  hdd_use.usage[0] = fixedOutput;

  // audio space
  cmd = "df | grep ^Data/Audio | awk '{print $3}'";
  output = &getCmdOutput(&cmd[0])[0];
  fixedOutput = output;
  strReplace(fixedOutput, "\n", "");
  hdd_use.usage[1] = fixedOutput;

  // documents space
  cmd = "df | grep ^Data/Dokumente | awk '{print $3}'";
  output = &getCmdOutput(&cmd[0])[0];
  fixedOutput = output;
  strReplace(fixedOutput, "\n", "");
  hdd_use.usage[2] = fixedOutput;

  // movies space
  cmd = "df | grep ^Data/Filme | awk '{print $3}'";
  output = &getCmdOutput(&cmd[0])[0];
  fixedOutput = output;
  strReplace(fixedOutput, "\n", "");
  hdd_use.usage[3] = fixedOutput;

  // program space
  cmd = "df | grep ^Data/Programme | awk '{print $3}'";
  output = &getCmdOutput(&cmd[0])[0];
  fixedOutput = output;
  strReplace(fixedOutput, "\n", "");
  hdd_use.usage[4] = fixedOutput;

  // game space
  cmd = "df | grep ^Data/Spiele | awk '{print $3}'";
  output = &getCmdOutput(&cmd[0])[0];
  fixedOutput = output;
  strReplace(fixedOutput, "\n", "");
  hdd_use.usage[5] = fixedOutput;
*/
}



void HDD::foo() {
	//readHDDTemperature();
	readHDDUsage();
}


/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/


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



void HDD::loadConfigFile(std::string configFile) {

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






// generic function to export a variable number of hdd elements (array size) in json file
void HDD::writeTemperatureJSONFile() {

  std::ofstream tempFile;
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
      std::string val;
      std::stringstream out;
      out << hdd_list[i].hdd_temp[j];
      tempFile << "               { \"title\" : \"" + hdd_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (hdd_position != 0) {
      for (int i=0; i < hdd_position; i++) {
        std::string val;
        std::stringstream out;
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

  std::ofstream tempFile;
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

  // for every load value print a line:
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

