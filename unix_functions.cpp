#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include "unix_functions.h"

using namespace std;


std::string getCmdOutput(char* cmd) {
  FILE* pipe = popen(cmd, "r");
  if ((!pipe) || (pipe == nullptr)) { 
    return "ERROR";
  }
  char buffer[128];
  std::string result = "";
  while(!feof(pipe)) {
    if(fgets(buffer, 128, pipe) != NULL) {
      result += buffer;
    }
  }
  if (pclose(pipe) == -1) {
	return "ERROR";
  }
  return result;
}



bool getDefaultEditor(std::string &output) {
  char *_defaultEditor;
	
  if ((_defaultEditor = getenv("EDITOR")) == nullptr) {
    std::string _editors[] = {"nano", "pico", "vim", "vi"};
    for (std::string &i : _editors) {
      std::string cmd = "which " + i;
      if (strcmp((_defaultEditor = &getCmdOutput(&cmd[0])[0]), "") != 0) {
        output.assign(_defaultEditor);
        strReplace(output, "\n", "");
        return true;
      }
    }
  } else {
    output.assign(_defaultEditor);
    return true;
  }

  return false;
}



std::string getReadableTime() {
  time_t currentTime;
  time(&currentTime);

  struct tm *readableTime = localtime(&currentTime);
  char readableStr[30];
  // strftime (buf, 30, "%a, %d %b %Y %H:%M:%S",  ptm);
  strftime (readableStr, 30, "%H:%M",  readableTime);
  
  return readableStr;
}



void strReplace(std::string& str, const std::string& oldStr, const std::string& newStr) {
  size_t pos = 0;
  while((pos = str.find(oldStr, pos)) != std::string::npos)
  {
     str.replace(pos, oldStr.length(), newStr);
     pos += newStr.length();
  }
}


std::string IntToStr(int value) {
  stringstream s;
  s << value;
  return s.str();
}


bool FileExists(const string& filename) {
  if (FILE *file = fopen(filename.c_str(), "r")) {
    fclose(file);
    return true;
  } else {
    return false;
  }   
}