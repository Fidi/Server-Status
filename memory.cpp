#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "memory.h"
#include "unix_functions.h"
#include "ini.h"

using namespace std;


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

Memory::Memory(string configFile){	
  loadConfigFile(configFile);
  initArray();
}

Memory::~Memory() {
  delete [] memory_list;
}





// function to save the systems temperature into array
void Memory::readMemoryStatus() {
  
  // read cpu temp values:
  std::vector<double> newVal;
  for (int i=0; i<memory_count; i++) {
    string mem_cmd = memory_cmd[i];
    const char* mem_output = &getCmdOutput(&mem_cmd[0])[0];
    newVal.push_back(atof(mem_output));
  }
  
  // save values into array
  setMemory(getReadableTime(), newVal);

  // write json file:
  writeMemoryJSONFile();
}



/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

// loads the ini-based config and sets the variables
void Memory::loadConfigFile(string configFile) {
  INI ini(configFile);

  // read array sizes:
  memory_elements = ini.readInt("Memory", "elements");

  // read cpu infos
  memory_count = ini.readInt("Memory", "count");
  for (int i=1; i<=memory_count; i++) {
    memory_cmd.push_back(ini.readString("Memory", "cmd" + IntToStr(i)));
    memory_desc.push_back(ini.readString("Memory", "desc" + IntToStr(i)));
  }

  file_path = ini.readString("General", "filepath");
}



// creates an array and fills it with empty values
void Memory::initArray() {
  // alloc memory for array
  memory_list = new data_memory[memory_elements];
  
  // init array with empty values
  for (int i = 0; i < memory_elements; i++) {
    memory_list[i].timestamp    = "0";

    for (int j=0; j<memory_count; j++) {
      memory_list[i].mem.push_back(0);
    }
  }

  memory_position = 0;
}






// Setter for memory
void Memory::setMemory(std::string time, std::vector<double> value) {
  if (value.size() == memory_count) {
    
    // save timestamp:
    memory_list[memory_position].timestamp = time;

    // get values:
    memory_list[memory_position].mem.clear();
    for (int i=0; i<memory_count; i++) {
      memory_list[memory_position].mem.push_back(value[i]);
    }

    // move pointer:
    if (memory_position < memory_elements-1) {
      memory_position++;
    } else {
      memory_position = 0;
    }

  }
}







// generic function to export a variable number of memory elements (array size) in json file
void Memory::writeMemoryJSONFile() {
  ofstream tempFile;
  tempFile.open(file_path + "Memory.json");
  tempFile << "{ \n";
  tempFile << "  \"graph\" : { \n";
  tempFile << "  \"title\" : \"Memory\", \n";
  tempFile << "  \"type\" : \"line\", \n";
  tempFile << "  \"refreshEveryNSeconds\" : 60, \n";
  tempFile << "  \"datasequences\" : [ \n";

  // for every load value print a line:
  //int elements = getArraySize(cpu_list[0].core_temp);//(sizeof(cpu_list[0].core_temp)/sizeof(*cpu_list[0].core_temp));
  for (int j=0; j < memory_count; j++) {
	
    tempFile << "      {";
    tempFile << "      \"title\": \"" << memory_desc[j] << + "\", \n";
    tempFile << "            \"datapoints\" : [ \n";

    for (int i=memory_position; i < memory_elements; i++) {
      string val;
      stringstream out;
      out << memory_list[i].mem[j];
      tempFile << "               { \"title\" : \"" + memory_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (memory_position != 0) {
      // print the remaining elements
      for (int i=0; i < memory_position; i++) {
        string val;
        stringstream out;
        out << memory_list[i].mem[j];
        tempFile << "               { \"title\" : \"" + memory_list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
      }
    }


    tempFile << "             ] \n";
    if (j == memory_count-1) {
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