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

		// save/load
		bool loadConfig(std::string filename);
		//bool saveConfig(std::string filename);
		
		//===================================================================================
		// READ
		//===================================================================================		
		
		// returns the version of the config file; can be used for compabibility checks
		std::string readVersion();
		// returns the filepath where output files shell be stored.
		std::string readFilepath();
		
		// return the distribution details
		std::string readApplicationType();
		std::string readServerAddress();
		int readServerPort();
		
		bool readSSL();
		std::string readCertFile();
		std::string readKeyFile();
		
		
		std::vector<std::string> readSections();
		
		// returns basic configuration for one :
		std::string readType(std::string section);
		bool readEnabled(std::string section);		
		int readInterval(std::string section);		
		int readElementCount(std::string section);
		bool readDelta(std::string section);
		std::string readInput(std::string section);
		std::string readOutput(std::string section);
		
		// returns sequence informations
		int readSequenceCount(std::string );
		std::string readSequenceCommand(std::string , int num);
		std::string readSequenceTitle(std::string , int num);
		std::string readSequenceColor(std::string , int num);
		
		// returns JSON header informations for one section:
		std::string readJSONFilename(std::string section);
		std::string readJSONTitle(std::string section);
		std::string readJSONType(std::string );
		int readJSONRefreshInterval(std::string );
		int readJSONyAxisMinimum(std::string );
		int readJSONyAxisMaximum(std::string );
		
		// CSV related information
		std::string readCSVFilename(std::string section);
		std::string readCSVTitle(std::string section);
		
		// other stuff:
		void showErrorLog();
		void performSecurityCheck(std::string filename);
	private:
		libconfig::Config ConfigFile;
		
		std::string ErrorCode;
};


#endif