#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "cpu.h"
#include "unix_functions.h"
#include "ini.h"

using namespace std;


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

CPU::CPU(){
}

CPU::CPU(string configFile){	
  loadConfigFile(configFile);
  initArray();
}

// destructor
CPU::~CPU() {
  delete [] cpu_list;
  delete [] load_list;
}



// function to save the systems temperature into array
void CPU::readCPUTemperature() {
  
  // save timestamp:
  cpu_list[cpu_position].timestamp = getReadableTime();

  // get values:
  cpu_list[cpu_position].cpu_temp.clear();
  for (int i=0; i<core_count; i++) {
    string core_cmd = cpu_cmd[i];
    const char* core_output = &getCmdOutput(&core_cmd[0])[0];
    cpu_list[cpu_position].cpu_temp.push_back(atof(core_output));
  }

  // move pointer:
  if (cpu_position < cpu_elements-1) {
    cpu_position++;
  } else {
    cpu_position = 0;
  }

  // write json file:
  writeTemperatureJSONFile();
}



// function to save the load avarage into array
void CPU::readLoadAverage() {
	
  // save timestamp:
  load_list[load_position].timestamp = getReadableTime();

  // get values:
  load_list[load_position].load_average.clear();
  for (int i=0; i<3; i++) {
    string la_cmd = load_cmd[i];
    const char* load_output = &getCmdOutput(&la_cmd[0])[0];
    load_list[load_position].load_average.push_back(atof(load_output));
  }

  // move pointer:
  if (load_position < load_elements-1) {
    load_position++;
  } else {
    load_position = 0;
  }

  // write json file:
  writeLoadJSONFile();
}


/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/


// creates an array and fills it with empty values
void CPU::initArray() {
  // alloc memory for array
  cpu_list = new data_cpu[cpu_elements];
  load_list = new data_load[load_elements];
  
  // init array with empty values
  for (int i = 0; i < cpu_elements; i++) {
    cpu_list[i].timestamp    = "0";

    for (int j=0; j<core_count; j++) {
      cpu_list[i].cpu_temp.push_back(0);
    }
  }

  for (int i = 0; i < cpu_elements; i++) {
    load_list[i].timestamp = "0";

    for (int j=0; j<3; j++) {
      load_list[i].load_average.push_back(0);
    }
  }

  cpu_position = 0;
  load_position = 0;
}




void CPU::loadConfigFile(string configFile) {
  INI ini(configFile);

  // read array sizes:
  cpu_elements = ini.readInt("CPU", "elements");
  load_elements = ini.readInt("Load", "elements");

  // read cpu infos
  core_count = ini.readInt("CPU", "count");
  for (int i=1; i<=core_count; i++) {
    cpu_cmd.push_back(ini.readString("CPU", "cmd" + IntToStr(i)));
  }

  for (int i=1; i<=3; i++) {
    load_cmd.push_back(ini.readString("Load", "cmd" + IntToStr(i)));
  }

  file_path = ini.readString("General", "filepath");
}






// generic function to export a variable number of cpu elements (array size) in json file
void CPU::writeTemperatureJSONFile() {
  ofstream tempFile;
  tempFile.open(file_path + "CPU_Temperature.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"CPU Temperature\", \n";
  tempFile << "  \"type\" : \"line\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 60, \n";
  tempFile << "  \"datasequences\" : [ \n";

  // for every load value print a line:
  //int elements = getArraySize(cpu_list[0].core_temp);//(sizeof(cpu_list[0].core_temp)/sizeof(*cpu_list[0].core_temp));
  for (int j=0; j < core_count; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"Core " << j+1 << + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

    for (int i=cpu_position; i < cpu_elements; i++) {
      string val;
      stringstream out;
      out << cpu_list[i].cpu_temp[j];
      tempFile << "               { \"title\" : \"" + cpu_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (cpu_position != 0) {
      // print the remaining elements
      for (int i=0; i < cpu_position; i++) {
        string val;
        stringstream out;
        out << cpu_list[i].cpu_temp[j];
        tempFile << "               { \"title\" : \"" + cpu_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
      }
    }


    tempFile << "             ] \n";
    if (j == core_count-1) {
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
  ofstream tempFile;
  tempFile.open(file_path + "Load.json");
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
  int elements = 3;
  for (int j=0; j < elements; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"" + load_desc[j] + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

    for (int i=load_position; i < load_elements; i++) {
      string val;
      stringstream out;
      out << load_list[i].load_average[j];
      tempFile << "               { \"title\" : \"" + load_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (load_position != 0) {
	  for (int i=0; i < load_position; i++) {
        string val;
	    stringstream out;
	    out << load_list[i].load_average[j];
	    tempFile << "               { \"title\" : \"" + load_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
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