#pragma once
#include <map>
#include <string>
#include <sstream>
#include <list>

#define NUM_A_PINS 8
#define NUM_D_PINS 12

class GenerationOptions
{
public:
	bool bPrettyPrint = true;									// Should generated code be properly spaced and easier to read (default set to TRUE in v2.1)
	bool bAddDebugStatements = false;							// #Defines DEBUG in generated code, causing Arduino to print out some helpful debug statements
	bool bDebugTrain = false;									// Adds debug statements to generated code to debug train pin functionality (added in v2.0)
	bool bDebugSkipRoutine = false;								// Adds debug statements to generated code to debug "Skip Routine" and randomization functionality (added in v2.0)
	bool bDebugLights = false;									// Adds debug statements to generated code to debug light functionality (added in v2.0)
	bool bRandomizeRoutineOrder = false;						// Should the generated code randomize the routine that's played every loop?
	bool bSwapOnOffValues = false;								// Should a "turn light on" command write LOW instead of HIGH?
	bool bUploadToMp3 = false;									// Should upload mp3 songs to the MP3 player
	bool bUseHalloweenMP3Controls = false;						// Should use legacy mp3 control scheme (simply power MP3 On to play, off to stop playing)
	bool bUseLowPrecisionTimes = false;							// Should use unsigned ints instead of unsigned longs for time stores
	bool bUseLegacyA6 = false;									// Preserves unknown feature of previous versions where A6 was hardcoded to be unused INPUT
	bool bUseLegacyA7 = false;									// Preserves unknown feature of previous versions where A7 was hardcoded to be unused INPUT_PULLUP
	bool bUseChristmasTrainSetup = true;						// Uses motor values specifically for the Christmas town scene setup (added in v2.2)

	std::string motionSensorPin = "";							// Pin used if the Arduino board has a motion detector; Generates the needed code if this is not empty
	std::string mp3SkipPin = "";								// Pin used to skip to the next song on the MP3 player
	std::string mp3VolumePin = "";								// Pin used to control the volume
	std::string trainPinLeft = "";								// Pin used to reset the train; Generates the needed code if this is not empty
	std::string trainPinRight = "";								// Pin used to reset the train; Generates the needed code if this is not empty
	std::string mp3DriveLetter = "";							// The drive letter where the plugged in MP3 player can be found
	std::string randomSeedPin = "";								// The pin to use as the initializer for random seed
	std::string motorVoltagePin = "";							// The pin used to read the voltage of a train motor (added in v2.0)
	std::string wifiSSID = "HawkswayBase";						// The SSID of the WiFi network to connect to (ESP32 only)
	std::string wifiPassword = "F4d29095dc";					// The password of the WiFi network to connect to (ESP32 only)

	unsigned long allLightsOnBlock = 0;
	unsigned long trainResetDuration = 0;						// The duration to run the train at the beginning of each power up

	bool IsTrainPin(std::string pin)
	{
		return trainPinLeft.compare(pin) == 0 || trainPinRight.compare(pin) == 0;
	}
};

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
	std::string label;
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


class CodeGenerator
{
private:
	GenerationOptions& Options;
	bool UseESP32;
	bool* APinUsage;
	bool* DPinUsage;
	bool* MotionSensePin;
	std::map<std::string, RoutineGUI>& Routines;
	RoutineOrderTracker& OrderTracker;
	std::ostringstream OutputString, RoutineArray;

public:
	CodeGenerator(GenerationOptions& options, bool* aPinUsage, bool* dPinUsage, bool* motionSensePin, std::map<std::string, RoutineGUI>& routines, RoutineOrderTracker& orderTracker)
		: Options(options), 
			UseESP32(!options.wifiSSID.empty()),
			APinUsage(aPinUsage),
			DPinUsage(dPinUsage),
			MotionSensePin(motionSensePin),
			Routines(routines),
			OrderTracker(orderTracker) {}

	// Main code generation function to build the entire output string
	std::string GenerateCode();

private:
	// Generates the #define statements at the top of the code
	void GenerateDefines();

	// Generates pin variables and #defines
	void GeneratePinSetup();

	// Generates the data structures for routines and lights
	void GenerateDataStructures();

	// Generate lights and routine variables
	void GenerateLightAndRoutineVariables();

	// Generate CheckLight function
	void GenerateCheckLightFunction();

	// Generate SkipToRoutine function
	void GenerateSkipToRoutineFunction();

	// Generate Light functions
	void GenerateLightFunctions();

	// Generate Arduino setup() function
	void GenerateSetupFunction();

	// Generate Arduino loop() function
	void GenerateLoopFunction();

	// Generate ESP32 Web Module code
	void GenerateESP32WebModuleCode();

	// Generate IsTrain helper function code
	void GenerateIsTrainFunction();

	// Helper to get routine label from either a custom label or routine name
	std::string GetRoutineLabel(const Routine& routine) const;
};

