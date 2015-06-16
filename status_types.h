#ifndef _statustypes_hpp_
#define _statustypes_hpp_

#include <string>
#include <vector>



// contains a timestamp and any number of values
struct _data_t {
  std::string timestamp;
  std::vector<double> value;
};
typedef struct _data_t data;



// differnt types of status information that serverstatus can read
enum _status_t {
  CPU,            // cpu temperature
  Load,           // load average (1, 5 and 15)
  HDD,            // disc temperature (smart required)
  Mount,          // disc size (free / used space)
  Memory,         // RAM status (active, inactive, free, ...)
  Network,        // Network in / out traffic
  None
};
typedef enum _status_t status;


// different graph types
enum _json_graph_t {
  JSON_LINE,               // line diagram
  JSON_BAR,                // bar diagram
  JSON_PIE,                // pie chart
  JSON_AUTO                // not specified (let parser decide)
};
typedef enum _json_graph_t json_graph;


// different ways to output the data
enum _data_output_t {
  OUT_JSON,                // export data to JSON file
  OUT_CSV,                 // export data to CSV file
  OUT_SOCKET,              // transmit data via socket to remote destination
  OUT_POST,                // use HTTP POST to send data to a server
  OUT_NONE                 // do nothing with the data... but why? 
};
typedef enum _data_output_t data_output;


// different input ways
enum _data_input_t {
  IN_CMD,                  // read data from bash command (or local program)
  IN_SOCKET,               // listen on socket for incoming data
  IN_NONE                  // I have no idea if this is ever needed?!
};
typedef enum _data_input_t data_input;




// get string that should be read from config file
std::string getStringFromType(status type);
status getTypeFromString(std::string type);


#endif