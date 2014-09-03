#ifndef _hdd_hpp_
#define _hdd_hpp_

#include <time.h>
#include <string>


// contains cpu temperature with their timestamp
struct data_hdd_t {
  std::string timestamp;
  std::vector<double> hdd_temp;
};
typedef struct data_hdd_t data_hdd;



struct data_usage_t {
  std::vector<std::string> mount;
};
typedef struct data_usage_t data_usage;





class HDD
{
  public:
    HDD();
    HDD(std::string configFile = "/usr/local/etc/serverstatus.conf");
    ~HDD();
 
	void readHDDTemperature();
	void readHDDUsage();
 
  private:
    int hdd_elements;
    int mount_elements;

    data_hdd *hdd_list;
    int hdd_position;

    int hdd_count;
    std::vector<std::string> hdd_desc;
    std::vector<std::string> hdd_cmd;

    data_usage hdd_usage;

    int mount_count;
    std::vector<std::string> mount_desc;
    std::vector<std::string> mount_cmd;

    std::string file_path;

    void initArray();
    void loadConfigFile(std::string configFile);
	
    void writeTemperatureJSONFile();
    void writeUsageJSONFile(); // TODO HTML export?!
};

#endif