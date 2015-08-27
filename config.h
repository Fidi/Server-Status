#ifndef _config_hpp_
#define _config_hpp_

#include <string.h>
#include <vector>
#include <libconfig.h++>


class config
{
	public:
		config(std::string filename);
		~config();
		
		//===================================================================================
		//=== General config settings
		//===================================================================================		
		
		// returns the version of the config file; can be used for compabibility checks
		std::string readVersion();
		
		// returns the filepath where output files shell be stored.
		std::string readFilepath();
		
		// return the distribution details
		std::string readApplicationType();
		
		// returns the port that a application set to server will listen on
		int readServerPort();
		
		// should encryption using OpenSSL be used
		bool readSSL();
		std::string readCertFile();
		std::string readKeyFile();
		
		// get a list of all sections that can be parsed using the following list
		std::vector<std::string> readSections();
		

		//===================================================================================
		//=== Section specific config settings
		//===================================================================================		
		
		// get the type such as load, cpu temperature or disc space
		std::string readType(std::string section);
		// should this section actually be used within the program
		bool readEnabled(std::string section);		
		// returns the interval in which the data a read
		int readInterval(std::string section);		
		// get the number of elements that are kept in memory
		int readElementCount(std::string section);
		// if false, absolute values will be printed; if true, the difference to the previous value
		bool readDelta(std::string section);
		
		// defines the input method for this section such as command or socket
		std::string readInput(std::string section);
		// defines the output method for this section such as json, csv or socket
		std::string readOutput(std::string section);
		
		// returns the number of sequences (e.g. load might use 3 sequences for 1m, 5m and 15m)
		int readSequenceCount(std::string section);
		// gets the command that will return a value for sequence "num" if executed
		std::string readSequenceCommand(std::string section, int num);
		// returns a title for sequence no. "num"
		std::string readSequenceTitle(std::string section, int num);
		// returns a color for sequence "num"
		std::string readSequenceColor(std::string section, int num);
		
		
		//===================================================================================
		//=== JSON specific config settings
		//===================================================================================
		
		// returns the JSON filename (without path)
		std::string readJSONFilename(std::string section);
		// returns the title of the JSON file
		std::string readJSONTitle(std::string section);
		// returns the graph type for this JSON file such as bar or line
		std::string readJSONType(std::string section);
		// defines how often the JSON file shall be refreshed on client side
		int readJSONRefreshInterval(std::string section);
		// specifies a minimum value for the x axis scale
		int readJSONyAxisMinimum(std::string section);
		// specifies a maximum value for the x axis scale
		int readJSONyAxisMaximum(std::string section);
		
		
		//===================================================================================
		//=== CSV specific config settings
		//===================================================================================
		
		// returns the CSV filename (without path)
		std::string readCSVFilename(std::string section);
		// returns the title of the CSV file
		std::string readCSVTitle(std::string section);
		
		
		
		//===================================================================================
		//=== Notification specific config settings
		//===================================================================================
		
		// returns the notification type
		std::string readNotificationType(std::string section);
		// returns the http post/get url
		std::string readNotificationURL(std::string section);
		// returns the notification title
		std::string readNotificationTitle(std::string section);
		
		
		
		//===================================================================================
		//=== Other stuff
		//===================================================================================
		
		// displays a error log of parsing the cfg file
		void showErrorLog();
		// checks sequence commands for possible dangers
		void performSecurityCheck(std::string filename);
	private:
		libconfig::Config ConfigFile;
		
		std::string ErrorCode;
		
		// loads the cfg file into the class
		bool loadConfig(std::string filename);
};


#endif