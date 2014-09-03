#include <iostream>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "ini.h"
#include "unix_functions.h"

using namespace std;


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/

INI::INI() {

}

INI::INI(string filename)
: fFileName(filename) {	
  fRootNode = new node;
  fRootNode->data = "File not found";
  fRootNode->next = 0;

  fPositionPointer = fRootNode;
  
  loadFromFile(fFileName);
}


INI::~INI() {
}


/*****************************************************************
**  SAVE/LOAD
*****************************************************************/
bool INI::loadFromFile(string filename) {
  if (FileExists(fFileName)) {
    ifstream fFile;
    fFile.open(fFileName);
    string temp;

    fFile >> temp;
    fPositionPointer->data = temp;

    for (string line; getline(fFile, line); )
    {
      if ((line != "") && (line != " ") && (line != "\n")) {
        fPositionPointer->next = new node;
        fPositionPointer = fPositionPointer->next;
        fPositionPointer->data = line;
        fPositionPointer->next = 0;
      }
    } 

    fFile.close();
    return true;
  }
  return false;
}


bool INI::saveToFile(string filename) {
  ofstream fFile;
  fFile.open(filename);
  node *position = fRootNode;

  do {
    fFile << position->data + "\n";
    position = position->next;
  }	while (position->next != 0);

  fFile.close();
  return true;
}


/*****************************************************************
**  READ
*****************************************************************/
int INI::readInt(string section, string key) {
	return atoi(getValue(section, key).c_str());	
}

long INI::readLong(string section, string key) {
	return atol(getValue(section, key).c_str());
}

double INI::readFloat(string section, string key) {
	return atof(getValue(section, key).c_str());
}


char INI::readChar(string section, string key) {
	string val = getValue(section, key);
	if (val.length() > 0) {
		return val.at(0); 
	} else {
		return ' ';
	}
}

string INI::readString(string section, string key) {
	return getValue(section, key);
}



/*****************************************************************
**  WRITE
*****************************************************************/
void INI::writeInt(string section, string key, int value) {
  stringstream s;
  s << value;
  setEntry(section, key, s.str());
}

void INI::writeLong(string section, string key, long value) {
  stringstream s;
  s << value;
  setEntry(section, key, s.str());
}

void INI::writeFloat(string section, string key, double value) {
  stringstream s;
  s << value;
  setEntry(section, key, s.str());
}

void INI::writeChar(string section, string key, char value) {
  stringstream s;
  s << value;
  setEntry(section, key, s.str());
}

void INI::writeString(string section, string key, string value) {
  setEntry(section, key, value);
}


/*****************************************************************
**
**  PRIVATE STUFF
**
*****************************************************************/

string INI::Uppercase(string val) {
  string res = val;
  for_each(res.begin(), res.end(), [](char& in){ in = ::toupper(in); });
  return res;
}




node* INI::getEntry(string section, string key) {
  fPositionPointer = fRootNode;
  node *res = 0;
  string k;
  int keylen = key.length();
  
  bool sectionExists = false;
  bool nextSection = false;
  do {
    if (Uppercase(fPositionPointer->data) == "[" + Uppercase(section) + "]") {
	  // found the matching section
      sectionExists = true;
      while ((fPositionPointer->next != 0) && (!nextSection)) {
        fPositionPointer = fPositionPointer->next;
        if (fPositionPointer->data.at(0) != '[') {
	      // entry in the matching section
          k = fPositionPointer->data;
          if ((k.length() >= keylen) && (Uppercase(k).substr(0, keylen) == Uppercase(key))) {
            res = fPositionPointer;
          }
        } else {
          nextSection = true;
        }
      }
    }	

    if (fPositionPointer->next != 0) {
      fPositionPointer = fPositionPointer->next;
    }
  }	while ((fPositionPointer->next != 0) && (!sectionExists));

  return res;
}

node* INI::setEntry(string section, string key, string value) {
  node *k = getEntry(section, key);
  if (k != 0) {
    // entry exists in section
    k->data = key + "=" + value;
    return k;	
  } else {
	// create new entry:
    node *sec = createSection(section);
    bool nextSection = false;

    while ((sec->next != 0) && (!nextSection)) {
	  string s = sec->next->data;
      if (s.at(0) == '[') { nextSection = true; }
      else { sec = sec->next; }
    }

    node *tmp = sec->next;
    sec->next = new node;
    sec = sec->next;
    sec->data = key + "=" + value;
    sec->next = tmp;

    return sec;
  }	
}


node* INI::getSection(string section) {
  node *position = fRootNode;
  
  bool sectionExists = false;
  while ((position->next != 0) && (!sectionExists)) {
    if (Uppercase(position->data) == "[" + Uppercase(section) + "]") {
      sectionExists = true;
      // found the matching section
      return position;
    }

    position = position->next;
  }	

  return 0;
}

node* INI::createSection(string section) {
  node *k = getSection(section);
  if (k != 0) {
	// section exists
	return k;
  } else {
	// append section
	fPositionPointer = getLastEntry();
    fPositionPointer->next = new node;
    fPositionPointer = fPositionPointer->next;
    fPositionPointer->data = "[" + section + "]";
    fPositionPointer->next = 0;

    return fPositionPointer;
  }
}


node* INI::getLastEntry() {
  node *position = fRootNode;
  while (position->next != 0) { 
    position = position->next; 
  }
  return position;
}


string INI::getValue(string section, string key) {
  node *k = getEntry(section, key);
  if (k != 0) {
    string res = k->data;
    int keylen = key.length();

    return res.substr(keylen+1, res.length()-(keylen+1));
  } else {
	return "0";
  }
}

void INI::printList() {
  node *pos = fRootNode;
  string tmp;
  while (pos->next != 0) {
	tmp = pos->data;
    printf("%s \n", tmp.c_str());
    pos = pos->next;
  }
  printf("%s \n", pos->data.c_str());
}


