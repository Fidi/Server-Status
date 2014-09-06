#ifndef _systat_hpp_
#define _systat_hpp_

#include <time.h>
#include <string>
#include <vector>


// generic container:
// contains a timestamp and any number of values
struct _data_t {
  std::string timestamp;
  std::vector<double> value;
};
typedef struct _data_t data;


enum _status_t {
  CPU,
  Load,
  HDD,
  Mount,
  Memory,
  None
};
typedef enum _status_t status;


enum _json_t {
  line,
  bar
};
typedef enum _json_t json;


class SystemStats
{
  public:
    SystemStats(status type = None, std::string configFile = "/usr/local/etc/serverstatus.conf");
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

    std::string _filepath;
    std::string _section;
    json _json_type;
    int _interval;


    bool loadConfigFile(std::string configFile);
    void initArray();

    std::string getSectionFromType(status type);

    void setValue(std::string time, std::vector<double> value);
 
    void writeJSONFile();
};


#endif