#ifndef _systat_hpp_
#define _systat_hpp_

#include <time.h>
#include <string>
#include <vector>



// contains a timestamp and any number of values
struct _data_t {
  std::string timestamp;
  std::vector<double> value;
};
typedef struct _data_t data;


// differnt types of status information that serverstatus can read
enum _status_t {
  CPU,            // cpu temperature
  Load,           // load average (1, 5 and 15)
  HDD,            // disc temperature (smart required)
  Mount,          // disc size (free / used space)
  Memory,         // RAM status (active, inactive, free, ...)
  Network,        // Network in / out traffic
  None
};
typedef enum _status_t status;


// different graph types (for iOS StatusBoard)
enum _json_t {
  line,
  bar,
  pie,
  none
};
typedef enum _json_t json;


class SystemStats
{
  public:
    SystemStats(status type, std::string configFile);
    ~SystemStats();

    void readStatus();
    bool loadFromFile();

  private:
    status type;
    int array_size;

    data *list = nullptr;
    int list_position;

    int element_count;
    std::vector<std::string> cmd;
    std::vector<std::string> description;
    std::vector<std::string> color;

    bool delta;
    std::vector<double> delta_abs_value;
    
    std::vector<std::string> distribution;
    int port;
    bool ssl;

    std::string filepath;
    std::string section;
    json json_type;
    int interval;
    int refresh_interval;


    bool loadConfigFile(std::string configFile);
    void initArray();

    void setValue(std::string time, std::vector<double> value);
    
    bool isReceiving(std::string &sender_ip, std::string &clientID);
    bool isSending(std::string &receiver_ip, std::string &clientID);
 
    void Inc(int &value);
    void writeJSONFile();
};


// get string that should be read from config file
std::string getStringFromType(status type);
status getTypeFromString(std::string type);


#endif