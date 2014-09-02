#ifndef _hdd_hpp_
#define _hdd_hpp_

#include <time.h>
#include <string>


// contains cpu temperature with their timestamp
struct data_hdd_t {
  std::string timestamp;
  double hdd_temp[6];	
};
typedef struct data_hdd_t data_hdd;

struct hdd_usage_t {
 std::string usage[6];
};
typedef struct hdd_usage_t hdd_usage;





class HDD
{
  public:
    HDD();
    HDD(int elements=30);
    //HDD(const HDD& a);  // do not expect this to be ever needed; but can easily be added
    ~HDD();
 
    void readHDDInfos();
		
    data_hdd* getArrayElement(int position);
    hdd_usage* getHDDUsagePointer();
 
  private:
    int hdd_elements;
    data_hdd *hdd_list;
    hdd_usage hdd_use;
    int array_position;

    std::string hdd_desc[6] = {"HDD 1", "HDD 2", "HDD 3", "HDD 4", "HDD 5", "SSD"};
    std::string usage_desc[6] = {"Free", "Audio", "Documents", "Movies", "Programs", "Games"};


    // read the systems hdd temperature
    void readCurrentTemperature();
    void readCurrentUsage();

    void initArray();
	
    void writeTemperatureJSONFile();
    void writeUsageJSONFile(); // TODO HTML export?!
};

#endif