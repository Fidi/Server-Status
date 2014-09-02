#include <iostream>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "hdd.h"
#include "unix_functions.h"



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
HDD::HDD()
: hdd_elements(30) 
{
  initArray();
}

// constructor with defined element count
HDD::HDD(int elements)
: hdd_elements(elements)
{	
  initArray();
}

// destructor
HDD::~HDD() {
  delete [] hdd_list;
}


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



// getter function for array element; returns pointer
data_hdd* HDD::getArrayElement(int position) {
  if((position >= 0) && (position < hdd_elements)) {
    return &hdd_list[position];
  } else {
    return NULL;
  }
}

hdd_usage* HDD::getHDDUsagePointer() {
  return &hdd_use;
}


/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/


// function to save the systems temperature into array
void HDD::readCurrentTemperature() {

  //Step 1: read the current hdd temperature
  for (int i = 0; i < (sizeof(hdd_list[array_position].hdd_temp)/sizeof(*hdd_list[array_position].hdd_temp)); i++) {
	std::string hddID;
	std::stringstream out;
	out << i;
	hddID = out.str();
	
    std::string cmd = "sudo smartctl -a /dev/ada" + hddID + " | awk '/Temperature_Celsius/{print $0}' | awk '{print $10}'";
    const char* output = &getCmdOutput(&cmd[0])[0];
    double temperature = atof(output);
    hdd_list[array_position].hdd_temp[i] = temperature;
  }
}


// function to save the load avarage into array
void HDD::readCurrentUsage() {
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

}





// creates an array and fills it with empty values
void HDD::initArray() {
  // alloc memory for array
  hdd_list = new data_hdd[hdd_elements];
  
  // init array with empty values
  for (int i = 0; i < hdd_elements; i++) {
    hdd_list[i].timestamp = "0";
    for (int j = 0; j < (sizeof(hdd_list[i].hdd_temp)/sizeof(*hdd_list[i].hdd_temp)); j++) {
	  hdd_list[i].hdd_temp[j] = 0;
    }
  }
  array_position = 0;
}




// generic function to export a variable number of hdd elements (array size) in json file
void HDD::writeTemperatureJSONFile() {
  std::ofstream tempFile;
  tempFile.open("HDD_Temperature.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"HDD Temperature\", \n";
  tempFile << "  \"type\" : \"line\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 60, \n";
  tempFile << "  \"datasequences\" : [ \n";

  // for every load value print a line:
  int elements = getArraySize(hdd_list[0].hdd_temp); //(sizeof(hdd_list[0].hdd_temp)/sizeof(*hdd_list[0].hdd_temp));
  for (int j=0; j < elements; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"" + hdd_desc[j] + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

	
    for (int i=array_position; i < hdd_elements; i++) {
      std::string val;
      std::stringstream out;
      out << hdd_list[i].hdd_temp[j];
      tempFile << "               { \"title\" : \"" + hdd_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (array_position != 0) {
      for (int i=0; i < array_position; i++) {
        std::string val;
        std::stringstream out;
        out << hdd_list[i].hdd_temp[j];
        tempFile << "               { \"title\" : \"" + hdd_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
      }
    }

    tempFile << "             ] \n";
    if (j == elements-1) {
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
  tempFile.open("HDD_Usage.json");
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
  int elements = getArraySize(hdd_use.usage); //(sizeof(hdd_use.usage)/sizeof(*hdd_use.usage));

  for (int j=0; j < elements; j++) {
	
    tempFile << "       { \"title\" : \"" + usage_desc[j] + "\", \"value\" : " + hdd_use.usage[j] + "}, \n";
   
  }

  tempFile << "        ] \n";
  tempFile << "    } \n";

  tempFile << "  ] \n";
  tempFile << "  } \n";
  tempFile << "} \n";
  tempFile.close();
}

