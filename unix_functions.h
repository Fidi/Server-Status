#ifndef _unix_helper_hpp_
#define _unix_helper_hpp_

// execute shell command and use shell output
std::string getCmdOutput(char* cmd);

// convert unix time to format HH:MM
std::string getReadableTime();

// replace all function
void strReplace(std::string& str, const std::string& oldStr, const std::string& newStr);



// save an array and its array pointer to a file
void saveArrayToFile(int array[], int array_pointer, int size, std::string fileName);
void saveArrayToFile(double array[], int array_pointer, int size, std::string fileName);


// reads a file into the submitted array; returns the array pointer (the next position to write at)
int loadArrayFromFile(int array[], int size, std::string fileName);
int loadArrayFromFile(double array[], int size, std::string fileName);

#endif