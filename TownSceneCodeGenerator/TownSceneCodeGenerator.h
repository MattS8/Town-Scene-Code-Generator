#pragma once
#ifndef TOWNSCENECODEGENERATOR
#define TOWNSCENECODEGENERATOR

#include "resource.h"
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <regex>
#include <ShellAPI.h>

// For TrimAllWhitespace
#include <algorithm> 
#include <cctype>
#include <locale>

// Constants
#define UP 1
#define DOWN 0
#define MAX_NUM_ROUTINES 20

// Classes and structs for Town Scene Code Generator:

// Information about a single light in town scene
class Light
{
public:
	std::string pin;
	std::string onTimes;
	std::string onTimesLowPrecision;
	std::string state;
	std::string name;
	unsigned int numberOfTimes;

	Light()
	{
		this->numberOfTimes = 0;
		this->name = "";
	}
};

// Information about a single routine
class Routine
{
public:
	std::string name;
	std::map<std::string, Light> lights;
	unsigned long endTime;
	std::string wavFilePath;
};

// Used to link GUI objects to a routine
class RoutineGUI
{
public:
	HWND title;
	HWND upButton;
	HWND downButton;
	HWND deleteButton;
	HWND editButton;
	Routine routine;
};

// Keeps track of the order the user wants the routines in
class RoutineOrderTracker
{
public:
	std::list<std::string> routineNames;

	std::string getRoutinesString(bool isHalloweenVariant)
	{
		std::string strRet = "";
		std::list<std::string>::iterator it = routineNames.begin();

		if (!isHalloweenVariant) {
			strRet.append("&").append(routineNames.back()).append(",");
		}

		while (it != routineNames.end()) {
			if (!isHalloweenVariant && std::next(it) == routineNames.end())
				break;

			strRet.append("&").append(*it).append(",");
			++it;
		}

		strRet = strRet.substr(0, strRet.size() - 1);

		return strRet;
	}

};

class GenerationOptions
{
public:
	bool bPrettyPrint = false;									// Should generated code be properly spaced and easier to read?
	bool bAddDebugStatements = false;							// #Defines DEBUG in generated code, causing Arduino to print out some helpful debug statements
	bool bRandomizeRoutineOrder = false;						// Should the generated code randomize the routine that's played every loop?
	bool bSwapOnOffValues = false;								// Should a "turn light on" command write LOW instead of HIGH?
	bool bUploadToMp3 = false;									// Should upload mp3 songs to the MP3 player
	bool bUseHalloweenMP3Controls = false;						// Should use legacy mp3 control scheme (simply power MP3 On to play, off to stop playing)
	bool bUseLowPrecisionTimes = false;							// Should use unsigned ints instead of unsigned longs for time stores
	bool bUseLegacyA6 = false;									// Preserves unknown feature of previous versions where A6 was hardcoded to be unused INPUT
	bool bUseLegacyA7 = false;									// Preserves unknown feature of previous versions where A7 was hardcoded to be unused INPUT_PULLUP

	std::string motionSensorPin = "";							// Pin used if the Arduino board has a motion detector; Generates the needed code if this is not empty
	std::string mp3SkipPin = "";								// Pin used to skip to the next song on the MP3 player
	std::string mp3VolumePin = "";								// Pin used to control the volume
	std::string trainPinLeft = "";								// Pin used to reset the train; Generates the needed code if this is not empty
	std::string mp3DriveLetter = "";							// The drive letter where the plugged in MP3 player can be found
	std::string randomSeedPin = "";								// The pin to use as the initializer for random seed

	unsigned long allLightsOnBlock = 0;
	unsigned long trainResetDuration = 0;						// The duration to run the train at the beginning of each power up

	bool IsTrainPin(std::string pin)
	{
		return trainPinLeft.compare(pin) == 0;
	}
};

// Global Variables

std::string OutputLogStr;
std::map<std::string, RoutineGUI> Routines;

//--------------------------------------------------------------------------- Debug Functions

void PrintToOutputLog(std::string statement)
{
	std::wstring outputStr(statement.begin(), statement.end());
	OutputDebugStringW(outputStr.c_str());
}

// Helper Functions

//---------------------------------------------------------------------------

std::string GetStringFromWindow(HWND hwnd)
{
	std::vector<wchar_t> buf;
	std::string temp;
	int len;

	len = GetWindowTextLengthW(hwnd) + 1;
	if (len > 1) {
		buf = std::vector<wchar_t>(len);
		GetWindowTextW(hwnd, &buf[0], len);
		temp = std::string(buf.begin(), buf.end());
		temp.pop_back();
		return temp;
	}

	return "";
}

unsigned long GetLongFromWindow(HWND hwnd)
{
	std::string windowText = GetStringFromWindow(hwnd);

	if (!windowText.empty()) {
		return (unsigned long)strtol(windowText.c_str(), NULL, 0);
	}
	else {
		return 0;
	}
}

//---------------------------------------------------------------------------

// Converts a time in m:ss.mmm to an unsigned long representing the number of milliseconds
unsigned long GetTimeMillis(std::string word)
{
	std::size_t found = word.find(":");		// First break in string
	std::size_t found2 = word.find(".");	// Second break in string

	// Get just the minutes string
	std::string minute = word.substr(0, found);
	minute.erase(0, min(minute.find_first_not_of('0'), minute.size() - 1));

	// Get just the seconds string
	std::string second = word.substr(found + 1, found2 != std::string::npos ? found2 - 2 : word.size() - 1);
	second.erase(0, min(second.find_first_not_of('0'), second.size() - 1));

	// Get just the milliseconds string
	std::string millis = found2 != std::string::npos ? word.substr(found2 + 1, word.size() - 1) : "0";
	millis.erase(0, min(millis.find_first_not_of('0'), millis.size() - 1));

	return (strtol(minute.c_str(), NULL, 0) * 60 + strtol(second.c_str(), NULL, 0)) * 1000 + strtol(millis.c_str(), NULL, 0);
}

//---------------------------------------------------------------------------

std::string trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
	{
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

//---------------------------------------------------------------------------

std::string GenerateRoutineName()
{
	std::ostringstream osName;
	osName << "routine_" << (Routines.size() + 1);

	return osName.str();
}

//---------------------------------------------------------------------------

bool IsValidVarName(std::string strFileName)
{
	std::regex reg("^[a-zA-Z_][a-zA-Z0-9_]*$");

	return std::regex_match(strFileName, reg);
}

//---------------------------------------------------------------------------

std::string TrimAllWhitespace(std::string str)
{
	size_t pos = str.find_first_of(" \t");

	while (pos != std::string::npos)
	{
		if (pos == 0)
		{
			if (str.length() < 2)
				break;
			str = str.substr(1, str.length());
		}
		else if (pos == str.length())
		{
			if (pos - 1 < 0)
				break;
			str = str.substr(0, str.length() - 1);
		}
		else
		{
			str = str.substr(0, pos).append(str.substr(pos + 1, str.length()));
		}
		pos = str.find_first_of(" \t");
	}


	// trim trailing spaces
	size_t endpos = str.find_last_not_of(" \t");
	size_t startpos = str.find_first_not_of(" \t");
	if (std::string::npos != endpos)
	{
		str = str.substr(0, endpos + 1);
		str = str.substr(startpos);
	}
	else {
		str.erase(std::remove(std::begin(str), std::end(str), ' '), std::end(str));
	}

	// trim leading spaces
	startpos = str.find_first_not_of(" \t");
	if (std::string::npos != startpos)
	{
		str = str.substr(startpos);
	}

	return str;
}

//---------------------------------------------------------------------------

void findAndReplaceAll(std::string & data, std::string toSearch, std::string replaceStr)
{
	// Get the first occurrence
	size_t pos = data.find(toSearch);

	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = data.find(toSearch, pos + replaceStr.size());
	}
}

#endif