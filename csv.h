#ifndef _csv_hpp_
#define _csv_hpp_

#include <string.h>
#include "status_types.h"

class CSV
{
  public:
    CSV(std::string configFile, std::string section);
    ~CSV();
    
    bool loadCSVfromFile(data output[], size_t array_size);
    bool writeCSVtoFile(data output[], size_t array_size, int position_pointer);

  private:
    
    std::string section;
    
    std::string filepath;
    std::string csv_filename;
    
    int sequence_count;
    int sequence_length;

    std::string csv_title;
    std::vector<std::string> sequence_title;
    std::vector<std::string> sequence_color;
    
    bool loadConfigFile(std::string configFile);
    
    // Inc and Dec implement some ring-pointer behaviour
    void Inc(int &value, size_t array_size);
    void Dec(int &value, size_t array_size);
};

#endif