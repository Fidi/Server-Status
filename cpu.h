#ifndef _cpu_hpp_
#define _cpu_hpp_

#include <time.h>
#include <string>
#include <vector>


// contains cpu temperature with their timestamp
struct data_cpu_t {
  std::string timestamp;
  std::vector<double> cpu_temp;
};
typedef struct data_cpu_t data_cpu;



// contains load avarage with its timestamp
struct data_load_t {
  std::string timestamp;
  std::vector<double> load_average;
};
typedef struct data_load_t data_load;





class CPU
{
  public:
    CPU();
    CPU(std::string configFile = "/usr/local/etc/serverstatus.conf");
    ~CPU();
 
    void readCPUTemperature();
    void readLoadAverage();

  private:
    int cpu_elements; 
    int load_elements;

    data_cpu *cpu_list;
    int cpu_position;

    data_load *load_list;
    int load_position;

    int core_count;
    std::vector<std::string> cpu_cmd;

    std::vector<std::string> load_cmd;
    std::string load_desc[3] = {"Load 1m", "Load 5m", "Load 15m"};

    std::string file_path;


    void initArray();
    void loadConfigFile(std::string configFile);
	
    void writeTemperatureJSONFile();
    void writeLoadJSONFile();
};


#endif