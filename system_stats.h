#ifndef _systat_hpp_
#define _systat_hpp_

#define __JSON__ true

#include <time.h>
#include "status_types.h"




// socket structure 
struct _socket_details_t {
  int port;
  std::string host_ip;
  bool ssl;
};
typedef struct _socket_details_t socket_details;




class SystemStats
{
  public:
    SystemStats(status type, std::string configFile);
    ~SystemStats();
    
    data_input input;
    socket_details input_details;
    
    data_output output;
    socket_details output_details;
    
    void readStatus();
    bool loadFromFile();

  private:
    std::string sConfigFile;
    
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
    
    std::vector<std::string> _input_details;
    std::vector<std::string> _output_details;
    int port;
    bool ssl;

    std::string filepath;
    std::string section;
    json_graph json_type;
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


#endif