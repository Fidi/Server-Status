#ifndef _memory_hpp_
#define _memory_hpp_

#include <time.h>
#include <string>
#include <vector>


// contains cpu temperature with their timestamp
struct data_memory_t {
  std::string timestamp;
  std::vector<double> mem;
};
typedef struct data_memory_t data_memory;



class Memory
{
  public:
    Memory(std::string configFile = "/usr/local/etc/serverstatus.conf");
    ~Memory();

    void readMemoryStatus();

  private:
    int memory_elements;

    data_memory *memory_list;
    int memory_position;

    int memory_count;
    std::vector<std::string> memory_cmd;
    std::vector<std::string> memory_desc;

    std::string file_path;


    void loadConfigFile(std::string configFile);
    void initArray();

    void setMemory(std::string time, std::vector<double> value);
 
    void writeMemoryJSONFile();
};


#endif