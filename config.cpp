#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include <string.h>
#include <libconfig.h++>
#include "config.h"
#include "unix_functions.h"

using namespace std;
using namespace libconfig;


/*****************************************************************
**
**  PUBLIC STUFF
**
*****************************************************************/
config::config(string filename) {
	this->ErrorCode = "";
	if (!loadConfig(filename)){
		printf("Error loading configuration file. \n\n");		
	}
}

config::~config() {
	
}



// load the configuration file 
bool config::loadConfig(string filename) {
	try
	{
		this->ConfigFile.readFile(filename.c_str());
		return true;
	}
	catch(const FileIOException &fioex)
	{
		// I/O Error
		this->ErrorCode += string("I/O error while reading file. \n");
		return false;;
	}
	catch(const ParseException &pex)
	{
		// Parse Error at 
		this->ErrorCode += string(pex.getFile()) + string(":") + string(to_string(pex.getLine())) + string(" - ") + string(pex.getError()) + string("\n");
		return false;
	}
}



/*****************************************************************
**
**  Application settings
**
*****************************************************************/
string config::readVersion() {
	try {
		return this->ConfigFile.lookup("version");
	} catch (const SettingNotFoundException &nfound) {
		return "version_error";
	}
}
string config::readFilepath() {
	try {
		string path = this->ConfigFile.lookup("filepath");
		if (path.length() == 0) {
			return "/usr/local/serverstatus/"; 
		} else if(path.at(path.length()-1) != '/'){
			path += "/";
		}
		return path;
	} catch (const SettingNotFoundException &nfound) {
		return "/usr/local/serverstatus/";
	}
}


string config::readApplicationType() {
	try {
		const Setting &s = this->ConfigFile.getRoot()["distribution"];
		string app_type;
		s.lookupValue("application", app_type);
		return app_type;
	} catch (const SettingNotFoundException &nfound) {
		return "server";
	}
}
int config::readServerPort() {
	try {
		const Setting &s = this->ConfigFile.getRoot()["distribution"];
		string s_port;
		s.lookupValue("server_port", s_port);
		return atoi(s_port.c_str());
	} catch (const SettingNotFoundException &nfound) {
		return 0;
	}
}


bool config::readSSL() {
	try {
		const Setting &s = this->ConfigFile.getRoot()["distribution"];
		bool ssl;
		s.lookupValue("ssl", ssl);
		return ssl;
	} catch (const SettingNotFoundException &nfound) {
		return false;
	}
}
string config::readCertFile(){
	try {
		const Setting &s = this->ConfigFile.getRoot()["distribution"];
		string cert;
		s.lookupValue("certificate", cert);
		return cert;
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}
string config::readKeyFile(){
	try {
		const Setting &s = this->ConfigFile.getRoot()["distribution"];
		string key;
		s.lookupValue("key", key);
		return key;
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}




/*****************************************************************
**
**  Section settings
**
*****************************************************************/
vector<string> config::readSections() {
	vector<string> v;
	
	try {
		const Setting &s = this->ConfigFile.getRoot()["sections"];
		for (int i = 0; i < s.getLength(); i++) {
			v.push_back(s[i]);
		}
	} catch (const SettingNotFoundException &nfound) {		
	}
	
	return v;
}



string config::readType(string section) {
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		string type;
		s.lookupValue("type", type);
		return type;
	} catch (const SettingNotFoundException &nfound) {
		return "none";
	}
}
bool config::readEnabled(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		bool enabled;
		s.lookupValue("enabled", enabled);
		return enabled;
	} catch (const SettingNotFoundException &nfound) {
		return true;
	}
}		
int config::readInterval(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		int interval;
		s.lookupValue("interval", interval);
		return interval;
	} catch (const SettingNotFoundException &nfound) {
		return 0;
	}
}	
int config::readElementCount(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		int element;
		s.lookupValue("elements", element);
		return element;
	} catch (const SettingNotFoundException &nfound) {
		return 30;
	}
}
bool config::readDelta(string section){
	bool delta = false;
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		s.lookupValue("delta", delta);
	} catch (const SettingNotFoundException &nfound) {
		return false;
	}
	return delta;
}
string config::readInput(string section) {
	try{
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		string in;
		s.lookupValue("input", in);
		return in;
	} catch (const SettingNotFoundException &nfound) {
		return "NONE";
	}
}
string config::readOutput(string section) {
	try{
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()];
		string out;
		s.lookupValue("output", out);
		return out;
	} catch (const SettingNotFoundException &nfound) {
		return "NONE";
	}
}



/*****************************************************************
**
**  Sequence settings
**
*****************************************************************/
int config::readSequenceCount(string section){
	int count = 0;
	try {
		const Setting &sec1 = this->ConfigFile.getRoot()[section.c_str()]["sequence"]["cmd"];
		if (sec1.getLength() > count) { count = sec1.getLength(); }		
		const Setting &sec2 = this->ConfigFile.getRoot()[section.c_str()]["sequence"]["title"];
		if (sec2.getLength() > count) { count = sec2.getLength(); }	
		return count;
	} catch (const SettingNotFoundException &nfound) {
		return count;
	}
}
string config::readSequenceCommand(string section, int num){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["sequence"]["cmd"];
		return s[num];
	} catch (const SettingNotFoundException &nfound) {
		return "echo 0";
	}
}
string config::readSequenceTitle(string section, int num){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["sequence"]["title"];
		return s[num];
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}
string config::readSequenceColor(string section, int num){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["sequence"]["colors"];
		return s[num];
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}




/*****************************************************************
**
**  JSON settings
**
*****************************************************************/
string config::readJSONFilename(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["json"];
		string filename;
		s.lookupValue("filename", filename);
		if (filename.substr(filename.find_last_of(".") + 1) != "json") {
			filename += ".json";
		}
		return filename;
	} catch (const SettingNotFoundException &nfound) {
		return section + ".json";
	}
}
string config::readJSONTitle(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["json"]["header"];
		string title;
		s.lookupValue("title", title);
		return title;
	} catch (const SettingNotFoundException &nfound) {
		return "No title found";
	}
}
string config::readJSONType(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["json"]["header"];
		string jtype;
		s.lookupValue("type", jtype);
		return jtype;
	} catch (const SettingNotFoundException &nfound) {
		return "line";
	}
}
int config::readJSONRefreshInterval(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["json"]["header"];
		int refresh;
		s.lookupValue("refreshEveryNSeconds", refresh);
		return refresh;
	} catch (const SettingNotFoundException &nfound) {
		return 300;
	}
}
int config::readJSONyAxisMinimum(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["json"]["header"]["yAxis"];
		int yMin;
		s.lookupValue("minValue", yMin);
		return yMin;
	} catch (const SettingNotFoundException &nfound) {
		return -42;
	}
}
int config::readJSONyAxisMaximum(string section){
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["json"]["header"]["yAxis"];
		int yMax;
		s.lookupValue("maxValue", yMax);
		return yMax;
	} catch (const SettingNotFoundException &nfound) {
		return -42;
	}
}



/*****************************************************************
**
**  CSV settings
**
*****************************************************************/
string config::readCSVFilename(string section) {
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["csv"];
		string filename;
		s.lookupValue("filename", filename);
		if (filename.substr(filename.find_last_of(".") + 1) != "csv") {
			filename += ".csv";
		}
		return filename;
	} catch (const SettingNotFoundException &nfound) {
		return section + ".csv";
	}
}
string config::readCSVTitle(string section) {
	try {
		const Setting &s = this->ConfigFile.getRoot()[section.c_str()]["csv"];
		string title;
		s.lookupValue("title", title);
		return title;
	} catch (const SettingNotFoundException &nfound) {
		return "No title found";
	}
}



/*****************************************************************
**
**  Other stuff
**
*****************************************************************/
void config::showErrorLog(){
	printf("Syntax and I/O check: ");
	if (this->ErrorCode != "") {
		printf("%s \n", this->ErrorCode.c_str());
	} else {
		printf("No errors found. \n\n");
	}
}
void config::performSecurityCheck(string filename){
	printf("Performing security check on configuration file \"%s\": \n", filename.c_str());
	
	// Modified check
	string cmd = "ls -lh " + filename + " | awk  '{print $6 \" \" $7 \", \" $8}'";
	const char* cmd_output1 = &getCmdOutput(&cmd[0])[0];
	printf("Last modified: %s", cmd_output1);
	cmd = "ls -lh " + filename + " | awk  '{print $3 \", \" $4}'";
	const char* cmd_output2 = &getCmdOutput(&cmd[0])[0];
	printf("Modified by: %s \n", cmd_output2);
	
	// check for potential dangerous commands
	printf("Checking for potential dangerous commands: \n");
	ifstream file(filename.c_str());
	string str; 
	bool riskFound = false;
	int lineNr = 1;
	while (getline(file, str))
	{
		if (str.find("sudo") != string::npos) {
			printf("Line %d: Command requires sudo privileges. \n", lineNr);
			riskFound = true;
		}
		
		if (str.find("rm ") != string::npos) {
			printf("Line %d: Command uses \"rm\". \n", lineNr);
			riskFound = true;
		}
		
		lineNr++;
	}
	
	if (!riskFound) {
		printf("No potential dangerous commands were found. \n");
	}
}