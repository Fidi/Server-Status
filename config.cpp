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
	fErrorCode = "";
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
		fConfigfile.readFile(filename.c_str());
		return true;
	}
	catch(const FileIOException &fioex)
	{
		// I/O Error
		fErrorCode += string("I/O error while reading file. \n");
		return false;;
	}
	catch(const ParseException &pex)
	{
		// Parse Error at 
		fErrorCode += string(pex.getFile()) + string(":") + string(to_string(pex.getLine())) + string(" - ") + string(pex.getError()) + string("\n");
		return false;
	}
}



string config::readVersion() {
	return fConfigfile.lookup("version");
}
string config::readFilepath() {
	string path = fConfigfile.lookup("filepath");
	if(path.at(path.length()-1) != '/'){
		path += "/";
	}
	return path;
}

string config::readApplicationType() {
	return fConfigfile.lookup("application");
}
string config::readServerAddress() {
	return fConfigfile.lookup("server_address");
}
string config::readServerPort() {
	return fConfigfile.lookup("server_port");
}



bool config::readEnabled(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()];
		bool enabled;
		section.lookupValue("enabled", enabled);
		return enabled;
	} catch (const SettingNotFoundException &nfound) {
		return false;
	}
}		
int config::readInterval(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()];
		int interval;
		section.lookupValue("interval", interval);
		return interval;
	} catch (const SettingNotFoundException &nfound) {
		return 0;
	}
}	
int config::readElementCount(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()];
		int element;
		section.lookupValue("elements", element);
		return element;
	} catch (const SettingNotFoundException &nfound) {
		return 30;
	}
}
bool config::readDelta(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()];
		bool delta;
		section.lookupValue("delta", delta);
		return delta;
	} catch (const SettingNotFoundException &nfound) {
		return false;
	}
}



string config::readJSONTitle(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["header"];
		string title;
		section.lookupValue("title", title);
		return title;
	} catch (const SettingNotFoundException &nfound) {
		return "No title found";
	}
}
string config::readJSONType(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["header"];
		string jtype;
		section.lookupValue("type", jtype);
		return jtype;
	} catch (const SettingNotFoundException &nfound) {
		return "line";
	}
}
int config::readJSONRefreshInterval(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["header"];
		int refresh;
		section.lookupValue("refreshEveryNSeconds", refresh);
		return refresh;
	} catch (const SettingNotFoundException &nfound) {
		return 300;
	}
}
int config::readJSONyAxisMinimum(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["header"]["yAxis"];
		int yMin;
		section.lookupValue("minValue", yMin);
		return yMin;
	} catch (const SettingNotFoundException &nfound) {
		return -42;
	}
}
int config::readJSONyAxisMaximum(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["header"]["yAxis"];
		int yMax;
		section.lookupValue("maxValue", yMax);
		return yMax;
	} catch (const SettingNotFoundException &nfound) {
		return -42;
	}
}




int config::readSequenceCount(string type){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["sequence"]["cmd"];
		return section.getLength();
	} catch (const SettingNotFoundException &nfound) {
		return 0;
	}
}
string config::readSequenceCommand(string type, int num){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["sequence"]["cmd"];
		return section[num];
	} catch (const SettingNotFoundException &nfound) {
		return "echo 0";
	}
}
string config::readSequenceTitle(string type, int num){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["sequence"]["title"];
		return section[num];
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}
string config::readSequenceColor(string type, int num){
	try {
		const Setting &section = fConfigfile.getRoot()[type.c_str()]["sequence"]["colors"];
		return section[num];
	} catch (const SettingNotFoundException &nfound) {
		return "-";
	}
}




void config::showErrorLog(){
	printf("Syntax and I/O check: ");
	if (fErrorCode != "") {
		printf("%s \n", fErrorCode.c_str());
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