#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "unix_functions.h"
#include "system_stats.h"

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

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}
