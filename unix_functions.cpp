#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <algorithm>

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


std::string getUsernameFromUID(int uid) {
  
    struct passwd* pw;

    if ((pw = getpwuid(uid)) == NULL) {
       return "unknown [uid " + to_string(uid) + "]";
    }
    
    return string(pw->pw_name) + " [uid " + to_string(uid) + "]";
    
    /*
    printf( "login name  %s\n", pw->pw_name );
    printf( "user id     %d\n", pw->pw_uid );
    printf( "group id    %d\n", pw->pw_gid );
    printf( "home dir    %s\n", pw->pw_dir );
    printf( "login shell %s\n", pw->pw_shell );
    */    
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


std::string trim(std::string s) {
  string x = s;
  x.erase(remove(x.begin(), x.end(), ' '), x.end());
  return x;
}

std::string IntToStr(int value) {
  stringstream s;
  s << value;
  return s.str();
}


// function to check if file exists
bool file_exists(const string& name) {
  struct stat buffer;   
  return (stat(name.c_str(), &buffer) == 0); 
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
