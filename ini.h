#ifndef _ini_hpp_
#define _ini_hpp_

#include <time.h>
#include <string>


struct node {
  std::string data;
  node *next;
};



class INI
{
  public:
    INI();
    INI(std::string filename);
    ~INI();

    // save/load
    bool loadFromFile(std::string filename);
    bool saveToFile(std::string filename);
	
    // read
    int readInt(std::string section, std::string key);
    long readLong(std::string section, std::string key);
    double readFloat(std::string section, std::string key);
    char readChar(std::string section, std::string key);
    std::string readString(std::string section, std::string key);

    // write
    void writeInt(std::string section, std::string key, int value);
    void writeLong(std::string section, std::string key, long value);
    void writeFloat(std::string section, std::string key, double value);
    void writeChar(std::string section, std::string key, char value);
    void writeString(std::string section, std::string key, std::string value);

  private:
    node *fRootNode;			//never rewrite this (unless you know why)
    node *fPositionPointer;

    std::string fFileName;

    std::string Uppercase(std::string val);

    node* getEntry(std::string section, std::string key);
    node* setEntry(std::string section, std::string key, std::string value);
    node* getSection(std::string section);
    node* createSection(std::string section);
    node* getLastEntry();
    std::string getValue(std::string section, std::string key);
    
    void printList();
};


#endif