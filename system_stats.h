#ifndef _systat_hpp_
#define _systat_hpp_

#define __JSON__ true
#define __CSV__ true

#include <time.h>
#include "status_types.h"

#if __JSON__
  #include "json.h"
#endif
#if __CSV__
  #include "csv.h"
#endif




// socket structure 
struct _socket_details_t {
  int port;
  std::string ip_address;
  bool ssl;
  std::string id;
};
typedef struct _socket_details_t socket_details;




class SystemStats
{
  public:
    SystemStats(std::string section, std::string configFile);
    ~SystemStats();
    
    bool readStatus();
    bool loadFromFile();

  private:
    std::string configFile;
    
    std::vector<std::string> cmd;
    
    data_input input;
    socket_details input_details;
    
    data_output output;
    socket_details output_details;
    
    int array_size;
    int element_count;

    data *list = nullptr;
    int list_position;
    
    std::string section;
    
    #if __JSON__
      JSON *json_class = nullptr;
    #endif
    #if __CSV__
      CSV *csv_class = nullptr;
    #endif


    bool loadConfigFile(std::string configFile);
    void initArray();

    bool setValue(std::string time, std::vector<double> value);
    void saveData();
    
    bool isReceiving(std::vector<std::string> input, std::string &sender_ip, std::string &clientID);
    bool isSending(std::vector<std::string> output, std::string &receiver_ip, std::string &clientID);
    
    void Inc(int &value);
    void Dec(int &value);
    
    data_input getInputFromString(std::string input);
    data_output getOutputFromString(std::string output);
};


#endif