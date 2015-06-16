#ifndef _json_hpp_
#define _json_hpp_

#include <string.h>
#include "status_types.h"

class JSON
{
  public:
    JSON(std::string configFile, std::string section);
    ~JSON();
    
    bool loadJSONfromFile(data output[], size_t array_size);
    bool writeJSONtoFile(data output[], size_t array_size, int position_pointer);

  private:
    
    std::string section;
    
    std::string filepath;
    std::string json_filename;
    
    int sequence_count;
    int sequence_length;

    std::vector<std::string> sequence_title;
    std::vector<std::string> sequence_color;

    bool delta;
    std::vector<double> absolute_value;

    json_graph graph_type;
    int refresh_interval;
    
    bool loadConfigFile(std::string configFile);
    
    // type conversions
    std::string getGraphTypeString(json_graph type);
    json_graph getGraphTypeFromString(std::string type);
    
    // Inc and Dec implement some ring-pointer behaviour
    void Inc(int &value, size_t array_size);
    void Dec(int &value, size_t array_size);
};

#endif