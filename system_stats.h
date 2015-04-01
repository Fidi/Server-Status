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
  Network         // Network in / out traffic
};
typedef enum _status_t status;


// different graph types (for iOS StatusBoard)
enum _json_t {
  line,
  bar,
  none
};
typedef enum _json_t json;


class SystemStats
{
  public:
    SystemStats(status type, std::string configFile);
    ~SystemStats();

    void readStatus();

  private:
    status _type;
    int _array_size;

    data *_list = nullptr;
    int _list_position;

    int _element_count;
    std::vector<std::string> _cmd;
    std::vector<std::string> _description;

    bool _delta;

    std::string _filepath;
    std::string _section;
    json _json_type;
    int _interval;
    int _refresh_interval;


    bool loadConfigFile(std::string configFile);
    void initArray();

    void setValue(std::string time, std::vector<double> value);
 
    void Inc(int &value);
    void writeJSONFile();
};


// get string that should be read from config file
std::string getSectionFromType(status type);


#endif