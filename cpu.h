#ifndef _cpu_hpp_
#define _cpu_hpp_

#include <time.h>
#include <string>


// contains cpu temperature with their timestamp
struct data_cpu_t {
  std::string timestamp;
  double core_temp[2];
  double load[3];
};
typedef struct data_cpu_t data_cpu;




class CPU
{
  public:
    CPU();
    CPU(int elements=30);
    //CPU(const CPU& a);  // do not expect this to be ever needed; but can easily be added
    ~CPU();
 
    void readCPUInfos();
		
    data_cpu* getArrayElement(int position);
 
  private:
    int cpu_elements;
    data_cpu *cpu_list;
    int array_position;


    std::string core_desc[2] = {"CPU 1", "CPU 2"};
    std::string load_desc[3] = {"Load 1m", "Load 5m", "Load 15m"};


    // read the systems cpu temperature
    void readCurrentTemperature();
    void readLoadAvarage();

    void initArray();
	
    void writeTemperatureJSONFile();
    void writeLoadJSONFile();
};


#endif