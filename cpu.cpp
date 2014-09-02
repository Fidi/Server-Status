#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "cpu.h"
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
CPU::CPU()
: cpu_elements(30) 
{
  initArray();
}

// constructor with defined element count
CPU::CPU(int elements)
: cpu_elements(elements)
{	
  initArray();
}

// destructor
CPU::~CPU() {
  delete [] cpu_list;
}


void CPU::readCPUInfos() {
  // get current time (UNIX format)
  cpu_list[array_position].timestamp = getReadableTime();

  readCurrentTemperature();
  readLoadAvarage();
	
  // move to next array element
  if (array_position < cpu_elements-1) {
    array_position++;
  } else {
    array_position = 0;
  }

  writeTemperatureJSONFile();
  writeLoadJSONFile();
}



// getter function for array element; returns pointer
data_cpu* CPU::getArrayElement(int position) {
  if((position >= 0) && (position < cpu_elements)) {
    return &cpu_list[position];
  } else {
    return NULL;
  }
}



/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/


// function to save the systems temperature into array
void CPU::readCurrentTemperature() {

  //Step 1: read the current systems temperature
  std::string cmd_core1 = "sysctl -n hw.acpi.thermal.tz0.temperature | sed 's/C//g'";
  const char* core1_output = &getCmdOutput(&cmd_core1[0])[0];
  double core1_temp = atof(core1_output);
  std::string cmd_core2 = "sysctl -n hw.acpi.thermal.tz1.temperature | sed 's/C//g'";
  const char* core2_output = &getCmdOutput(&cmd_core2[0])[0];
  double core2_temp = atof(core2_output);
	
  //Step 2: write the data into array
  cpu_list[array_position].core_temp[0] = core1_temp;
  cpu_list[array_position].core_temp[1] = core2_temp;

}


// function to save the load avarage into array
void CPU::readLoadAvarage() {
	
  //Step 1: read the current load avarage
  std::string cmd_load1 = "uptime | awk '{print $(NF-2)}' | sed 's/,//'";
  const char* load1_output = &getCmdOutput(&cmd_load1[0])[0];
  double load1 = atof(load1_output);
  std::string cmd_load5 = "uptime | awk '{print $(NF-1)}' | sed 's/,//'";
  const char* load5_output = &getCmdOutput(&cmd_load5[0])[0];
  double load5 = atof(load5_output);
  std::string cmd_load15 = "uptime | awk '{print $(NF)}'";
  const char* load15_output = &getCmdOutput(&cmd_load15[0])[0];
  double load15 = atof(load15_output);
		
  //Step 2: write the data into array
  cpu_list[array_position].load[0] = load1;
  cpu_list[array_position].load[1] = load5;
  cpu_list[array_position].load[2] = load15;

}





// creates an array and fills it with empty values
void CPU::initArray() {
  // alloc memory for array
  cpu_list = new data_cpu[cpu_elements];
  
  // init array with empty values
  for (int i = 0; i < cpu_elements; i++) {
    cpu_list[i].timestamp    = "0";
    cpu_list[i].core_temp[0] = 0;
    cpu_list[i].core_temp[1] = 0;
    cpu_list[i].load[0]      = 0;
    cpu_list[i].load[1]      = 0;
    cpu_list[i].load[2]      = 0;
  }
  array_position = 0;
}




// generic function to export a variable number of cpu elements (array size) in json file
void CPU::writeTemperatureJSONFile() {
  std::ofstream tempFile;
  tempFile.open("CPU_Temperature.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"CPU Temperature\", \n";
  tempFile << "  \"type\" : \"line\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 60, \n";
  tempFile << "  \"datasequences\" : [ \n";

  // for every load value print a line:
  int elements = getArraySize(cpu_list[0].core_temp);//(sizeof(cpu_list[0].core_temp)/sizeof(*cpu_list[0].core_temp));
  for (int j=0; j < elements; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"" + core_desc[j] + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

    for (int i=array_position; i < cpu_elements; i++) {
      std::string val;
      std::stringstream out;
      out << cpu_list[i].core_temp[j];
      tempFile << "               { \"title\" : \"" + cpu_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (array_position != 0) {
      // print the remaining elements
      for (int i=0; i < array_position; i++) {
        std::string val;
        std::stringstream out;
        out << cpu_list[i].core_temp[j];
        tempFile << "               { \"title\" : \"" + cpu_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
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



// generic function to export all load values (3) as json file
void CPU::writeLoadJSONFile() {
  std::ofstream tempFile;
  tempFile.open("Load.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"Load Average\", \n";
  tempFile << "  \"type\" : \"line\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 60, \n";
  tempFile << "  \"yAxis\" : { \n";
  tempFile << "         \"minValue\" : 0, \n";
  tempFile << "         \"maxValue\" : 3 \n";
  tempFile << "   }, \n";   
  tempFile << "  \"datasequences\" : [ \n";

  // for every load value print a line:
  int elements = getArraySize(cpu_list[0].load); //(sizeof(cpu_list[0].load)/sizeof(*cpu_list[0].load));
  for (int j=0; j < elements; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"" + load_desc[j] + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

    for (int i=array_position; i < cpu_elements; i++) {
      std::string val;
      std::stringstream out;
      out << cpu_list[i].load[j];
      tempFile << "               { \"title\" : \"" + cpu_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (array_position != 0) {
	  for (int i=array_position; i < cpu_elements; i++) {
        std::string val;
	    std::stringstream out;
	    out << cpu_list[i].load[j];
	    tempFile << "               { \"title\" : \"" + cpu_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
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