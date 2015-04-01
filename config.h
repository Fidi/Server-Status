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
		
		// returns basic configuration for one type:
		bool readEnabled(std::string type);		
		int readInterval(std::string type);		
		int readElementCount(std::string type);
		bool readDelta(std::string type);
		
		// returns JSON header informations for one type:
		std::string readJSONTitle(std::string type);
		std::string readJSONType(std::string type);
		int readJSONRefreshInterval(std::string type);
		int readJSONyAxisMinimum(std::string type);
		int readJSONyAxisMaximum(std::string type);
		
		int readSequenceCount(std::string type);
		std::string readSequenceCommand(std::string type, int num);
		std::string readSequenceTitle(std::string type, int num);
		std::string readSequenceColor(std::string type, int num);
	/*
		// read
		int readInt(std::string section, std::string key);
		long readLong(std::string section, std::string key);
		double readFloat(std::string section, std::string key);
		char readChar(std::string section, std::string key);
		std::string readString(std::string section, std::string key);

		// write
		void writeInt(std::string section, std::string key, int value);
		void writeLong(std::string section, std::string key, long value);
		void writeFloat(std::string section, std::string key, double value);
		void writeChar(std::string section, std::string key, char value);
		void writeString(std::string section, std::string key, std::string value);
*/
	private:
		libconfig::Config fConfigfile;
};


#endif