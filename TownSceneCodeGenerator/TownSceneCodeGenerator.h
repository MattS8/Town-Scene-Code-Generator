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

// Constants
#define UP 1
#define DOWN 0

// Classes and structs for Town Scene Code Generator:

// Information about a single light in town scene
class Light
{
public:
	std::string pin;
	std::string onTimes;
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
};

// Used to link GUI objects to a routine
class RoutineGUI
{
public:
	HWND title;
	HWND upButton;
	HWND downButton;
	Routine routine;
};

// Keeps track of the order the user wants the routines in
class RoutineOrderTracker
{
public:
	std::list<std::string> routineNames;

	std::string getRoutinesString()
	{
		std::string strRet = "";
		std::list<std::string>::iterator it;
		for (it = routineNames.begin(); it != routineNames.end(); ++it)
		{
			strRet.append("&").append(*it).append(",");
		}

		return strRet;
	}

};

class GenerationOptions
{
public:
	bool bPrettyPrint = false;									// Should generated code be properly spaced and easier to read?
	bool bAddDebugStatements = false;							// #Defines DEBUG in generated code, causing Arduino to print out some helpful debug statements
	bool bRandomizeRoutineOrder = false;						// Should the generated code randomize the routine that's played every loop?

	std::string motionSensorPin = "";							// Pin used if the Arduino board has a motion detector; Generates the needed code if this is not empty
	std::string mp3SkipPin = "";								// Pin used to skip to the next song on the MP3 player
	std::string mp3VolumePin = "";								// Pin used to control the volume
	std::string trainPinLeft = "";								// Pin used to reset the train; Generates the needed code if this is not empty

	bool IsTrainPin(std::string pin)
	{
		return trainPinLeft.compare(pin) == 0;
	}
};

// Global Variables

std::string OutputLogStr;
std::map<std::string, RoutineGUI> Routines;

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

#endif