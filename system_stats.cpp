#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "system_stats.h"
#include "unix_functions.h"
#include "ini.h"

using namespace std;


// helper function to get a string from the status type
string getSectionFromType(status type) {
  string res;
  switch (type) {
    case CPU: res     = "CPU";     break;
    case Load: res    = "Load";    break;
    case HDD: res     = "HDD";     break;
    case Mount: res   = "Mount";   break;
    case Memory: res  = "Memory";  break;
    case Network: res = "Network"; break;
    default: throw "No status type submitted."; break;
  }
  
  return res;
}


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

SystemStats::SystemStats(status type, string configFile){
  // set the type (cpu, load, hdd, mount or memory):
  _type = type;
  
  // now init the generic class with the config file
  if (loadConfigFile(configFile)) {
    initArray();
  } 
}

SystemStats::~SystemStats() {
  delete [] _list;
}




// function to save the systems temperature into array
void SystemStats::readStatus() {
  
  // read requested values:
  std::vector<double> newVal;
  for (int i = 0; i < _element_count; i++) {
    string cmd = _cmd[i];
    const char* cmd_output = &getCmdOutput(&cmd[0])[0];
    newVal.push_back(atof(cmd_output));
  }
  
  // save values into array
  setValue(getReadableTime(), newVal);

  // write json file:
  writeJSONFile();
}



/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

// loads the ini-based config and sets the variables
bool SystemStats::loadConfigFile(string configFile) {
	
  if (FileExists(configFile)) {
	_section = getSectionFromType(_type);
    INI ini(configFile);

    // read array sizes:
    _array_size = ini.readInt(_section, "elements");

    // read cpu infos
    _element_count = ini.readInt(_section, "count");
    for (int i=1; i<=_element_count; i++) {
      _cmd.push_back(ini.readString(_section, "cmd" + IntToStr(i)));
      _description.push_back(ini.readString(_section, "desc" + IntToStr(i)));
    }

    if (ini.readString(_section, "graphtype") == "bar") {
      _json_type = bar;
    } else {
      _json_type = line;
    }
    if (ini.readInt(_section, "delta") == 1) {
      _delta = true;
    } else {
      _delta = false;
    }

    _interval = ini.readInt(_section, "interval");
    _filepath = ini.readString("General", "filepath");
    return true;
  }

  return false;
}




// creates an array and fills it with empty values
void SystemStats::initArray() {

  // alloc memory for array
  _list = new data[_array_size];
  
  // init array with empty values
  for (int i = 0; i < _array_size; i++) {
    _list[i].timestamp    = "0";

    for (int j = 0; j < _element_count; j++) {
      _list[i].value.push_back(0);
    }
  }

  // set pointer
  _list_position = 0;
}



data SystemStats::getPreviousData() {
  // move pointer:
  int _prevPosition;
  if (_list_position == 0) {
    _prevPosition =  _array_size - 1;
  } else {
    _prevPosition = _list_position - 1;
  }

  return _list[_prevPosition];
}





// Array setter:
void SystemStats::setValue(std::string time, std::vector<double> value) {
	
  if (value.size() == _element_count) {
    
    // save timestamp:
    _list[_list_position].timestamp = time;

    // get values:
    _list[_list_position].value.clear();
    for (int i=0; i<_element_count; i++) {
      _list[_list_position].value.push_back(value[i]);
    }

    // move pointer:
    if (_list_position < _array_size - 1) {
      _list_position++;
    } else {
      _list_position = 0;
    }

  }
}




// generic json export function
void SystemStats::writeJSONFile() {
  ofstream _out_file;
  _out_file.open(_filepath + _section + ".json");
  _out_file << "{ \n";
  _out_file << "  \"graph\" : { \n";
  _out_file << "  \"title\" : \"" + _section + "\", \n";
  string _graphtype;
  switch (_json_type) {
    case bar: _graphtype = "bar"; break;
    default: _graphtype = "line"; break;
  }
  _out_file << "  \"type\" : \"" + _graphtype + "\", \n";
  string _update;
  switch (_interval) {
    case 1: _update = "60"; break;
    case 5: _update = "150"; break;
    default: _update = "300"; break;
  }
  _out_file << "  \"refreshEveryNSeconds\" : " + _update + ", \n";
  _out_file << "  \"datasequences\" : [ \n";

  // for every load value print a line:
  for (int j = 0; j < _element_count; j++) {
	
    _out_file << "      {";
    _out_file << "      \"title\": \"" << _description[j] << + "\", \n";
    _out_file << "            \"datapoints\" : [ \n";


    double _val;
    for (int i = _list_position; i < _array_size; i++) {
      string val;
      stringstream out;
      _val = _list[i].value[j];
      if (_delta) { if (i == 0) { _val -= _list[_array_size-1].value[j]; } else { _val -= _list[i-1].value[j]; } }
      out << _val;
      _out_file << "               { \"title\" : \"" + _list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
    }

    if (_list_position != 0) {
      // print the remaining elements
      for (int i=0; i < _list_position; i++) {
        string val;
        stringstream out;
        _val = _list[i].value[j];
        if (_delta) { if (i == 0) { _val -= _list[_array_size-1].value[j]; } else { _val -= _list[i-1].value[j]; } }
        out << _val;
        _out_file << "               { \"title\" : \"" + _list[i].timestamp + "\", \"value\" : " + out.str() + "}, \n";
      }
    }


    _out_file << "             ] \n";
    if (j == _element_count - 1) {
	  _out_file << "      } \n";
    } else {
      _out_file << "      }, \n";
    }

  }

  _out_file << "  ] \n";
  _out_file << "  } \n";
  _out_file << "} \n";
  _out_file.close();
}