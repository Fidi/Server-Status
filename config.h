#ifndef _config_hpp_
#define _config_hpp_

#include <string.h>
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
		std::string readServerPort();
		
		// returns basic configuration for one type:
		bool readEnabled(std::string type);		
		int readInterval(std::string type);		
		int readElementCount(std::string type);
		bool readDelta(std::string type);
		std::string readDistribution(std::string type);
		
		// returns JSON header informations for one type:
		std::string readJSONTitle(std::string type);
		std::string readJSONType(std::string type);
		int readJSONRefreshInterval(std::string type);
		int readJSONyAxisMinimum(std::string type);
		int readJSONyAxisMaximum(std::string type);
		
		// returns sequence informations
		int readSequenceCount(std::string type);
		std::string readSequenceCommand(std::string type, int num);
		std::string readSequenceTitle(std::string type, int num);
		std::string readSequenceColor(std::string type, int num);
		
		// other stuff:
		void showErrorLog();
		void performSecurityCheck(std::string filename);
	private:
		libconfig::Config ConfigFile;
		
		std::string ErrorCode;
};


#endif