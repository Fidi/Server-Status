#ifndef _hdd_hpp_
#define _hdd_hpp_

#include <time.h>
#include <string>


// contains cpu temperature with their timestamp
struct data_hdd_t {
  std::string timestamp;
  //double hdd_temp[6];	
  std::vector<double> hdd_temp;
};
typedef struct data_hdd_t data_hdd;



struct data_usage_t {
 //std::string usage[6];
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

    void foo();
 
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

    //int array_position;

    //std::string hdd_desc[6] = {"HDD 1", "HDD 2", "HDD 3", "HDD 4", "HDD 5", "SSD"};
    //std::string usage_desc[6] = {"Free", "Audio", "Documents", "Movies", "Programs", "Games"};


    void initArray();
    void loadConfigFile(std::string configFile);
	
    void writeTemperatureJSONFile();
    void writeUsageJSONFile(); // TODO HTML export?!
};

#endif