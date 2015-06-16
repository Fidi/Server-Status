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



string config::readVersion() {
	return this->ConfigFile.lookup("version");
}
string config::readFilepath() {
	string path = this->ConfigFile.lookup("filepath");
	if(path.at(path.length()-1) != '/'){
		path += "/";
	}
	return path;
}


string config::readApplicationType() {
	try {
		const Setting &section = this->ConfigFile.getRoot()["distribution"];
		string app_type;
		section.lookupValue("application", app_type);
		return app_type;
	} catch (const SettingNotFoundException &nfound) {
		return "NONE";
	}
}
string config::readServerAddress() {
	try {
		const Setting &section = this->ConfigFile.getRoot()["distribution"];
		string s_addr;
		section.lookupValue("server_adress", s_addr);
		return s_addr;
	} catch (const SettingNotFoundException &nfound) {
		return "127.0.0.1";
	}
}
int config::readServerPort() {
	try {
		const Setting &section = this->ConfigFile.getRoot()["distribution"];
		string s_port;
		section.lookupValue("server_port", s_port);
		return atoi(s_port.c_str());
	} catch (const SettingNotFoundException &nfound) {
		return 0;
	}
}


bool config::readSSL() {
	try {
		const Setting &section = this->ConfigFile.getRoot()["distribution"];
		bool ssl;
		section.lookupValue("ssl", ssl);
		return ssl;
	} catch (const SettingNotFoundException &nfound) {
		return false;
	}
}
string config::readCertFile(){
	try {
		const Setting &section = this->ConfigFile.getRoot()["distribution"];
		string cert;
		section.lookupValue("certificate", cert);
		return cert;
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}
string config::readKeyFile(){
	try {
		const Setting &section = this->ConfigFile.getRoot()["distribution"];
		string key;
		section.lookupValue("key", key);
		return key;
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}



string config::readType(string section) {
	try {
		const Setting &sec = this->ConfigFile.getRoot()[section.c_str()];
		string type;
		sec.lookupValue("type", type);
		return type;
	} catch (const SettingNotFoundException &nfound) {
		return "none";
	}
}
bool config::readEnabled(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()];
		bool enabled;
		section.lookupValue("enabled", enabled);
		return enabled;
	} catch (const SettingNotFoundException &nfound) {
		return true;
	}
}		
int config::readInterval(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()];
		int interval;
		section.lookupValue("interval", interval);
		return interval;
	} catch (const SettingNotFoundException &nfound) {
		return 0;
	}
}	
int config::readElementCount(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()];
		int element;
		section.lookupValue("elements", element);
		return element;
	} catch (const SettingNotFoundException &nfound) {
		return 30;
	}
}
bool config::readDelta(string type){
	bool delta = false;
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()];
		section.lookupValue("delta", delta);
	} catch (const SettingNotFoundException &nfound) {
		return false;
	}
	return delta;
}
string config::readInput(string type) {
	try{
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()];
		string in;
		section.lookupValue("input", in);
		return in;
	} catch (const SettingNotFoundException &nfound) {
		return "NONE";
	}
}
string config::readOutput(string type) {
	try{
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()];
		string out;
		section.lookupValue("output", out);
		return out;
	} catch (const SettingNotFoundException &nfound) {
		return "NONE";
	}
}



string config::readJSONFilename(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"];
		string filename;
		section.lookupValue("filename", filename);
		if (filename.substr(filename.find_last_of(".") + 1) != "json") {
			filename += ".json";
		}
		return filename;
	} catch (const SettingNotFoundException &nfound) {
		return type + ".json";
	}
}
string config::readJSONTitle(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["header"];
		string title;
		section.lookupValue("title", title);
		return title;
	} catch (const SettingNotFoundException &nfound) {
		return "No title found";
	}
}
string config::readJSONType(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["header"];
		string jtype;
		section.lookupValue("type", jtype);
		return jtype;
	} catch (const SettingNotFoundException &nfound) {
		return "line";
	}
}
int config::readJSONRefreshInterval(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["header"];
		int refresh;
		section.lookupValue("refreshEveryNSeconds", refresh);
		return refresh;
	} catch (const SettingNotFoundException &nfound) {
		return 300;
	}
}
int config::readJSONyAxisMinimum(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["header"]["yAxis"];
		int yMin;
		section.lookupValue("minValue", yMin);
		return yMin;
	} catch (const SettingNotFoundException &nfound) {
		return -42;
	}
}
int config::readJSONyAxisMaximum(string type){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["header"]["yAxis"];
		int yMax;
		section.lookupValue("maxValue", yMax);
		return yMax;
	} catch (const SettingNotFoundException &nfound) {
		return -42;
	}
}




int config::readSequenceCount(string type){
	int count = 0;
	try {
		const Setting &sec1 = this->ConfigFile.getRoot()[type.c_str()]["cmd"];
		if (sec1.getLength() > count) { count = sec1.getLength(); }		
		const Setting &sec2 = this->ConfigFile.getRoot()[type.c_str()]["json"]["sequence"]["title"];
		if (sec2.getLength() > count) { count = sec2.getLength(); }	
		return count;
	} catch (const SettingNotFoundException &nfound) {
		return count;
	}
}
string config::readSequenceCommand(string type, int num){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["cmd"];
		return section[num];
	} catch (const SettingNotFoundException &nfound) {
		return "echo 0";
	}
}
string config::readSequenceTitle(string type, int num){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["sequence"]["title"];
		return section[num];
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}
string config::readSequenceColor(string type, int num){
	try {
		const Setting &section = this->ConfigFile.getRoot()[type.c_str()]["json"]["sequence"]["colors"];
		return section[num];
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}




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