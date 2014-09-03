#ifndef _unix_helper_hpp_
#define _unix_helper_hpp_

// execute shell command and use shell output
std::string getCmdOutput(char* cmd);

// convert unix time to format HH:MM
std::string getReadableTime();

// replace all function
void strReplace(std::string& str, const std::string& oldStr, const std::string& newStr);

std::string IntToStr(int value);

#endif