
#include "stdafx.h"
#include "CodeGenerator.h"
#include <cctype>

// /////////////////
// Helper Functions
// /////////////////
#include <string>
#include <cctype>

std::string FormatName(const std::string& input) {
	std::string out;
	out.reserve(input.size() * 3);

	bool capitalizeNext = true;

	for (size_t i = 0; i < input.size(); ++i) {
		char c = input[i];

		// Underscore → space
		if (c == '_') {
			out.push_back(' ');
			capitalizeNext = true;
			continue;
		}

		// CamelCase → add space before capital letters
		if (i > 0 && std::isupper(static_cast<unsigned char>(c))) {
			out.push_back(' ');
			capitalizeNext = true;
		}

		// Multi-digit number block
		if (std::isdigit(static_cast<unsigned char>(c))) {
			out.push_back(' ');

			// capture entire number run
			size_t start = i;
			while (i < input.size() && std::isdigit(static_cast<unsigned char>(input[i]))) {
				out.push_back(input[i]);
				i++;
			}

			out.append(" - ");
			capitalizeNext = true;

			i--; // offset the for-loop's increment
			continue;
		}

		// Normal letters
		if (capitalizeNext && std::isalpha(static_cast<unsigned char>(c))) {
			out.push_back(std::toupper(static_cast<unsigned char>(c)));
		}
		else {
			out.push_back(c);
		}

		capitalizeNext = false;
	}

	return out;
}


std::string CodeGenerator::GetRoutineLabel(const Routine& routine) const
{
	if (!routine.label.empty())
		return routine.label;
	
	return FormatName(routine.name);
}

// -------- GENERATE DEFINES -------- //
void CodeGenerator::GenerateDefines()
{
	OutputString << "#define ulong unsigned long\r\n"
		<< "#define ON " << (Options.bSwapOnOffValues ? 0 : 1) << "\r\n"
		<< "#define OFF " << (Options.bSwapOnOffValues ? 1 : 0) << "\r\n"
		<< "#define P_ALL_OFF -1\r\n"
		<< "#define P_ALL_ON -2\r\n";
	// Remap pins for esp32 boards
	if (UseESP32) {
		if (Options.bPrettyPrint)
			OutputString << "// ----- Digital pins remapped to ESP32 GPIOs -----" << "\r\n";
		OutputString << "#define D2 23" << "\r\n"
			<< "#define D3 32" << "\r\n"
			<< "#define D4 33" << "\r\n"
			<< "#define D5 25" << "\r\n"
			<< "#define D6 26" << "\r\n"
			<< "#define D7 27" << "\r\n"
			<< "#define D8 14" << "\r\n"
			<< "#define D9 12" << "\r\n"
			<< "#define D10 13" << "\r\n"
			<< "#define D11 22" << "\r\n"
			<< "#define D12 15" << "\r\n"
			<< "#define D13 19" << "\r\n";
		if (Options.bPrettyPrint)
			OutputString << "\r\n// ----- Nano analog pin numbers remapped as general I/O -----" << "\r\n";
		OutputString << "#define A0 2" << "\r\n"
			<< "#define A1 4" << "\r\n"
			<< "#define A2 16" << "\r\n"
			<< "#define A3 17" << "\r\n"
			<< "#define A4 5" << "\r\n"
			<< "#define A5 18" << "\r\n"
			<< "#define A6 39" << (Options.bPrettyPrint ? "   // ADC1_CH0 (real analog)" : "") << "\r\n"
			<< "#define A7 36" << (Options.bPrettyPrint ? "   // ADC1_CH3 (random seed floating input)" : "") << "\r\n";
	}
	else {
		OutputString << "#define D2 2\r\n" << "#define D3 3\r\n" << "#define D4 4\r\n" << "#define D5 5\r\n" << "#define D6 6\r\n" << "#define D7 7\r\n" << "#define D8 8\r\n";
		OutputString << "#define D9 9\r\n" << "#define D10 10\r\n" << "#define D11 11\r\n" << "#define D12 12\r\n" << "#define D13 13\r\n";
	}

	if (Options.bPrettyPrint)
		OutputString << "\r\n";
	if (!Options.motionSensorPin.empty())
		OutputString << "#define PMotionSense " << Options.motionSensorPin << "\r\n";
	OutputString << "#define MP3SkipPin " << Options.mp3SkipPin << "\r\n";
	if (!Options.mp3VolumePin.empty())
		OutputString << "#define MP3VolumePin " << Options.mp3VolumePin << "\r\n";
	if (!Options.trainPinLeft.empty())
		OutputString << "#define TrainPin " << Options.trainPinLeft << "\r\n";
	if (Options.bDebugLights)
		OutputString << "#define DEBUG\r\n";
	if (Options.bDebugTrain)
		OutputString << "#define DEBUG_TRAIN\r\n";
	if (Options.bDebugSkipRoutine)
		OutputString << "#define DEBUG_SKIP_ROUTINE\r\n";
}

// -------- GENERATE PIN SETUP -------- //
void CodeGenerator::GeneratePinSetup()
{
	OutputString << "bool bAllLightsOn = false;\r\n#define bRandomizeRoutineOrder " << (Options.bRandomizeRoutineOrder ? "true" : "false") << "\r\n";
	int numLights = 0;
	int temp = 0;
	int i;
	for (i = 0; i < NUM_A_PINS; i++)
		if (APinUsage[i] && !Options.IsTrainPin(std::string("A").append(std::to_string(i))))
			numLights++;
	for (i = 0; i < NUM_D_PINS; i++)
		if (DPinUsage[i] && !Options.IsTrainPin(std::string("D").append(std::to_string(i + 2))))
			numLights++;
	OutputString << "#define NUM_LIGHTS " << numLights << "\r\n";
	OutputString << "#define NUM_ROUTINES " << Routines.size() << "\r\n";
	OutputString << "int AllLights[NUM_LIGHTS] = {";
	if (APinUsage[0] && !Options.IsTrainPin("A0"))
		OutputString << "A0" << (++temp < numLights ? "," : "");
	if (APinUsage[1] && !Options.IsTrainPin("A1"))
		OutputString << "A1" << (++temp < numLights ? "," : "");
	if (APinUsage[2] && !Options.IsTrainPin("A2"))
		OutputString << "A2" << (++temp < numLights ? "," : "");
	if (APinUsage[3] && !Options.IsTrainPin("A3"))
		OutputString << "A3" << (++temp < numLights ? "," : "");
	if (APinUsage[4] && !Options.IsTrainPin("A4"))
		OutputString << "A4" << (++temp < numLights ? "," : "");
	if (APinUsage[5] && !Options.IsTrainPin("A5"))
		OutputString << "A5" << (++temp < numLights ? "," : "");
	if (!Options.bUseLegacyA6)
		if (APinUsage[6] && !Options.IsTrainPin("A6"))
			OutputString << "A6" << (++temp < numLights ? "," : "");
	if (!Options.bUseLegacyA7)
		if (APinUsage[7] && !Options.IsTrainPin("A7"))
			OutputString << "A7" << (++temp < numLights ? "," : "");

	if (DPinUsage[0] && !Options.IsTrainPin("D2"))
		OutputString << "D2" << (++temp < numLights ? "," : "");
	if (DPinUsage[1] && !Options.IsTrainPin("D3"))
		OutputString << "D3" << (++temp < numLights ? "," : "");
	if (DPinUsage[2] && !Options.IsTrainPin("D4"))
		OutputString << "D4" << (++temp < numLights ? "," : "");
	if (DPinUsage[3] && !Options.IsTrainPin("D5"))
		OutputString << "D5" << (++temp < numLights ? "," : "");
	if (DPinUsage[4] && !Options.IsTrainPin("D6"))
		OutputString << "D6" << (++temp < numLights ? "," : "");
	if (DPinUsage[5] && !Options.IsTrainPin("D7"))
		OutputString << "D7" << (++temp < numLights ? "," : "");
	if (DPinUsage[6] && !Options.IsTrainPin("D8"))
		OutputString << "D8" << (++temp < numLights ? "," : "");
	if (DPinUsage[7] && !Options.IsTrainPin("D9"))
		OutputString << "D9" << (++temp < numLights ? "," : "");
	if (DPinUsage[8] && !Options.IsTrainPin("D10"))
		OutputString << "D10" << (++temp < numLights ? "," : "");
	if (DPinUsage[9] && !Options.IsTrainPin("D11"))
		OutputString << "D11" << (++temp < numLights ? "," : "");
	if (DPinUsage[10] && !Options.IsTrainPin("D12"))
		OutputString << "D12" << (++temp < numLights ? "," : "");
	if (DPinUsage[11] && !Options.IsTrainPin("D13"))
		OutputString << "D13";
	OutputString << "};\r\n";

	// Even when not randomizing, we need these because my code is dumb
	OutputString << "bool all_used_up = false;\r\nbool used_routine[NUM_ROUTINES];\r\n";

	// Motor current variables
	if (!Options.trainPinLeft.empty()) {
		OutputString
			<< "uint8_t motor_current_limit = " << (Options.bUseChristmasTrainSetup ? "10" : "35") << ";\r\n"
			<< "uint8_t motor_current_limit_L2R = " << (Options.bUseChristmasTrainSetup ? "10" : "24") << ";\r\n"
			<< "uint8_t motor_current_limit_R2L = " << (Options.bUseChristmasTrainSetup ? "10" : "24") << ";\r\n"
			<< "int avgdly = 500;" << (Options.bPrettyPrint ? " // micro seconds delay between motor current readings for averaging" : "") << "\r\n"
			<< "int avg_count = 10;" << (Options.bPrettyPrint ? " // number of readings to average for motor current" : "") << "\r\n";
	}

	// NEW CODE GENERATED
	OutputString << "ulong StartTime = 0, DeltaTime = 0;\r\nint CurrentRoutine = 0;\r\n";

	// ESP32 WEB SKIP VARIABLES
	if (UseESP32) {
		OutputString << "int ForcedNextRoutine = -1;\r\n";
	}
	OutputString << "\r\n";
}

// -------- GENERATE DATA STRUCTURES -------- //
void CodeGenerator::GenerateDataStructures()
{
	OutputString
		<< "typedef struct OnTime\r\n"
		<< "{\r\n"
		<< "	" << (Options.bUseLowPrecisionTimes ? "unsigned int Start;\r\n	unsigned int End;" : "unsigned long Start;\r\n	unsigned long End;") << "\r\n"
		<< "} OnTime;\r\n"
		<< "\r\n"
		<< "typedef struct Light\r\n"
		<< "{\r\n"
		<< "	int Pin; \r\n"
		<< "	OnTime* Times;\r\n"
		<< "	int NumberOfOnTimes;\r\n"
		<< "	int State;\r\n"
		<< "} Light;\r\n"
		<< (Options.bPrettyPrint ? "\r\n" : "")
		<< "typedef struct Routine\r\n"
		<< "{\r\n"
		<< "	Light** Lights;\r\n"
		<< "	int NumberOfLights;\r\n"
		<< "	" << (Options.bUseLowPrecisionTimes ? "unsigned int" : "unsigned long") << " RoutineTime;\r\n"
		<< "} Routine;\r\n"
		<< (UseESP32 ? "volatile bool WebSkipRequested = false;\r\n\r\n" : "\r\n");
}

// -------- GENERATE LIGHT AND ROUTINE VARIABLES -------- //
void CodeGenerator::GenerateLightAndRoutineVariables()
{
	std::string comma = Options.bPrettyPrint ? ", " : ",";

	std::map<std::string, Light>::iterator itLights;
	std::map<std::string, RoutineGUI>::iterator itRoutines;
	for (itRoutines = Routines.begin(); itRoutines != Routines.end(); ++itRoutines)
	{
		std::ostringstream osLightArray;
		OutputString << "//" << itRoutines->second.routine.name << "\r\n";
		for (itLights = itRoutines->second.routine.lights.begin(); itLights != itRoutines->second.routine.lights.end(); ++itLights)
		{
			OutputString << "OnTime " << itRoutines->second.routine.name << "_" << itLights->second.name << "_OnTimes[" << itLights->second.numberOfTimes << "] = {" << (Options.bUseLowPrecisionTimes ? itLights->second.onTimesLowPrecision : itLights->second.onTimes) << ";\r\n";
			OutputString << "Light " << itRoutines->second.routine.name << "_" << itLights->second.name << " = {" << itLights->second.pin << comma << itRoutines->second.routine.name;
			OutputString << "_" << itLights->second.name << "_OnTimes" << comma << itLights->second.numberOfTimes << comma << "OFF};\r\n";
			if (Options.bPrettyPrint)
				OutputString << "\r\n";

			osLightArray << "&" << itRoutines->second.routine.name << "_" << itLights->second.name << comma;
		}
		OutputString << "Light* " << itRoutines->second.routine.name << "_Lights[" << itRoutines->second.routine.lights.size() << "] = {";
		OutputString << osLightArray.str().substr(0, osLightArray.str().size() - (Options.bPrettyPrint ? 2 : 1)) << "};\r\n"; //CHECK  THIS
		OutputString << "Routine " << itRoutines->second.routine.name << " = {" << itRoutines->second.routine.name << "_Lights" << comma << itRoutines->second.routine.lights.size() << comma;
		OutputString << (Options.bUseLowPrecisionTimes ? itRoutines->second.routine.endTime / 100 : itRoutines->second.routine.endTime) << "};\r\n";
		if (Options.bPrettyPrint)
			OutputString << "\r\n";

		//RoutineArray << "&" << itRoutines->second.routine.name << comma;
	}

	for (std::list<std::string>::iterator itter = OrderTracker.routineNames.begin(); itter != OrderTracker.routineNames.end(); ++itter) {
		RoutineArray << "&" << *itter << comma;
	}
	std::string routinesStr = RoutineArray.str();
	OutputString << "Routine* " << "routines[NUM_ROUTINES] = {" << routinesStr.substr(0, routinesStr.size()-2) << "};\r\n\r\n"; //CHECK  THIS
}

// -------- GENERATE CHECK LIGHT FUNCTION -------- //
void CodeGenerator::GenerateCheckLightFunction()
{
	if (Options.bPrettyPrint) {
		OutputString
			<< "/** \r\n"
			<< " * Turns a light on if DeltaTime is within one of the light's \"on times\". \r\n"
			<< " * Otherwise, the light is turned off.\r\n"
			<< " *   - light: The light to check\r\n"
			<< " *   Returns: Whether the light was turned on\r\n"
			<< " **/\r\n";
	}
	OutputString 
		<< "bool CheckLight(Light* light)\r\n"
		<< "{\r\n"
		<< "    int i;\r\n"
		<< "    for (i = 0; i < light->NumberOfOnTimes; i++)\r\n"
		<< "    {\r\n"
		<< "        if (light->Pin == P_ALL_OFF || light->Pin == P_ALL_ON)\r\n"
		<< "            return false;\r\n"
		<< "        if (DeltaTime >= "
		<< (Options.bUseLowPrecisionTimes
			? "((ulong)light->Times[i].Start * 100)"
			: "light->Times[i].Start")
		<< " && DeltaTime < "
		<< (Options.bUseLowPrecisionTimes
			? "((ulong)light->Times[i].End * 100)"
			: "light->Times[i].End")
		<< ")\r\n"
		<< "        {\r\n"
		<< "            #ifdef DEBUG\r\n"
		<< "            if (light->State == OFF) \r\n"
		<< "            {\r\n"
		<< "                Serial.print(\"Turning on light : \");\r\n"
		<< "                Serial.println(light->Pin);\r\n"
		<< "            }\r\n"
		<< "            light->State = ON;\r\n"
		<< "            #endif\r\n"
		<< "            digitalWrite(light->Pin, ON);\r\n"
		<< "            return true;\r\n"
		<< "        }\r\n"
		<< "    }\r\n"
		<< "    digitalWrite(light->Pin, OFF);\r\n"
		<< "\r\n"
		<< "    #ifdef DEBUG\r\n"
		<< "    if (light->State == ON)\r\n"
		<< "    {\r\n"
		<< "        Serial.print(\"Turning off light : \");\r\n"
		<< "        Serial.println(light->Pin);\r\n"
		<< "    }\r\n"
		<< "    light->State = OFF;\r\n"
		<< "    #endif\r\n"
		<< "    return false;\r\n"
		<< "}\r\n"
		<< "\r\n";
}

// -------- GENERATE SKIP TO ROUTINE FUNCTION -------- //
void CodeGenerator::GenerateSkipToRoutineFunction()
{
	if (Options.bUseHalloweenMP3Controls)
		return;

	if (Options.bPrettyPrint) {
		OutputString
			<< "/** Applies the proper number of Skip commands to the MP3 player in order to go from the current\r\n"
			<< " * 	track to the desired track.\r\n"
			<< " *   Returns: array position of next routine\r\n"
			<< " **/\r\n";
	}
	OutputString 
		<< "int SkipToRoutine()\r\n"
		<< "{"
		<< "	int i;\r\n"
		<< "	int nextRoutine = CurrentRoutine;\r\n";

	std::string tabs = "	";
	if (UseESP32) {
		if (Options.bPrettyPrint) {
			OutputString
				<< "	// ============================================================\r\n"
				<< "	// 1) If a specific routine was requested, honor that first\r\n"
				<< "	// ============================================================\r\n";
		}
		OutputString
			<< "	int numberOfSkips = 1;\r\n"
			<< "	if (ForcedNextRoutine != -1) {\r\n"
			<< "		nextRoutine = ForcedNextRoutine;\r\n"
			<< "		ForcedNextRoutine = -1;" << (Options.bPrettyPrint ? "	// Consume the override\r\n" : "\r\n")
			<< "\r\n"
			<< (Options.bPrettyPrint ? "	// Compute how many MP3 skips we need to get from CurrentRoutine\r\n" : "")
			<< (Options.bPrettyPrint ? "	// to nextRoutine, wrapping around NUM_ROUTINES.\r\n" : "")
			<< "		if (nextRoutine == CurrentRoutine) {\r\n"
			<< "			numberOfSkips = 0;" << (Options.bPrettyPrint ? "	// same routine: no MP3 skip\r\n" : "\r\n")
			<< "		} else if (nextRoutine > CurrentRoutine) {\r\n"
			<< "			numberOfSkips = nextRoutine - CurrentRoutine;\r\n"
			<< "		} else {\r\n"
			<< "			numberOfSkips = NUM_ROUTINES - CurrentRoutine + nextRoutine;\r\n"
			<< "		}\r\n";
			if (Options.bPrettyPrint) {
				OutputString
					<< "\r\n"
					<< "		// NOTE: we do NOT touch used_routine[] here,\r\n"
					<< "		// so your non-repeating random order is not disturbed by a manual jump.\r\n";
			}
			OutputString
				<< "	}\r\n"
				<< "	else {\r\n";
			if (Options.bPrettyPrint) {
				OutputString
					<< "		// ============================================================\r\n"
					<< "		// 2) Normal automatic behavior (original logic)\r\n"
					<< "		// ============================================================\r\n"; 
			}
			tabs = "		";
	}

	// v2.0 - new non-repeating randomization implementation
	if (Options.bPrettyPrint) {
		OutputString
			<< tabs << "// Determine if all routines have been used FIRST\r\n";
	}
	OutputString 
		<< tabs << "bool allRoutinesUsed = true;\r\n"
		<< tabs << "for (i = 0; i < NUM_ROUTINES; i++) {\r\n"
		<< tabs << "    if (!used_routine[i]) {\r\n"
		<< tabs << "        allRoutinesUsed = false;\r\n"
		<< tabs << "        break;\r\n"
		<< tabs << "    }\r\n"
		<< tabs << "}\r\n"
		<< "\r\n"
		<< tabs << "if (allRoutinesUsed) {\r\n"
		<< tabs << "    for (i = 0; i < NUM_ROUTINES; i++) {\r\n"
		<< tabs << "        used_routine[i] = false;\r\n"
		<< tabs << "    }\r\n"
		<< tabs << "}\r\n"
		<< tabs << "nextRoutine = bRandomizeRoutineOrder\r\n"
		<< tabs << "    ? CurrentRoutine\r\n"
		<< tabs << "    : CurrentRoutine + 1 == NUM_ROUTINES ? 0 : CurrentRoutine + 1;\r\n"
		<< tabs << "int numberOfSkips = 1;\r\n"
		<< "\r\n"
		<< "if (bRandomizeRoutineOrder && NUM_ROUTINES > 1) {\r\n"
		<< "    while (nextRoutine == CurrentRoutine || used_routine[nextRoutine])\r\n"
		<< "        nextRoutine = random(0, NUM_ROUTINES);\r\n"
		<< "    used_routine[nextRoutine] = true;\r\n"
		<< "    numberOfSkips = nextRoutine < CurrentRoutine\r\n"
		<< "        ? NUM_ROUTINES - CurrentRoutine + nextRoutine\r\n"
		<< "        : nextRoutine - CurrentRoutine;\r\n"
		<< "\r\n"
		<< tabs << "    #ifdef DEBUG_SKIP_ROUTINE\r\n"
		<< tabs << "    Serial.print(\"Current routine :\");\r\n"
		<< tabs << "    Serial.println(CurrentRoutine);\r\n"
		<< tabs << "    Serial.print(\"Routine \");\r\n"
		<< tabs << "    Serial.print(nextRoutine);\r\n"
		<< tabs << "    Serial.print(\" selected(skipping \");\r\n"
		<< tabs << "    Serial.print(numberOfSkips);\r\n"
		<< tabs << "    Serial.println(\" times)\");\r\n"
		<< tabs << "    #endif\r\n"
		<< tabs << "}\r\n";
	if (UseESP32) {
		OutputString
			<< "	}\r\n";
		if (Options.bPrettyPrint) {
			OutputString
				<< "	// ============================================================\r\n"
				<< "	// 3) Perform MP3 skip pulses (for both auto and forced jumps)\r\n"
				<< "	// ============================================================\r\n";
		}
	}
	OutputString
		<< "	for (int i = 0; i < numberOfSkips; i++) {\r\n"
		<< "		#ifdef DEBUG_SKIP_ROUTINE\r\n"
		<< "		Serial.print(\" skipping...\");\r\n"
		<< "		#endif\r\n"
		<< "	    digitalWrite(MP3SkipPin, HIGH);\r\n"
		<< "	    delay(150);\r\n"
		<< "	    digitalWrite(MP3SkipPin, LOW);\r\n"
		<< "	    delay(250);\r\n"
		<< "	}\r\n"
		<< "	#ifdef DEBUG_SKIP_ROUTINE\r\n"
		<< "	Serial.println(\"Done skipping!\");\r\n"
		<< "	#endif\r\n";
	OutputString 
		<< "    return nextRoutine;\r\n"
		<< "}";

	OutputString << "\r\n";
}

// -------- GENERATE LIGHT FUNCTIONS -------- //
void CodeGenerator::GenerateLightFunctions()
{
	// Turn All Lights
	OutputString << "/** Turns all lights on or off.\r\n *	- state: ON or OFF\r\n **/\r\nvoid TurnAllLights(int state)\r\n{\r\n	for (int i=0; i < NUM_LIGHTS; i++)	\r\n	{\r\n		#ifdef DEBUG\r\n		Serial.print(\"Turning light \");\r\n		Serial.print(AllLights[i]);\r\n		Serial.println(state == ON ? \" ON\" : \" OFF\");\r\n		#endif\r\n		digitalWrite(AllLights[i], state);	\r\n	}\r\n}\r\n";
	OutputString << "\r\n";

	// All Lights On 
	OutputString << "/** Checks to see if all lights should be turned on or not.\r\n *	- allOnLight: Light pointer containing all OnTimes for All Lights On\r\n **/\r\nbool AllLightsOn(Light* allOnLight)\r\n{\r\n	for (int i = 0; i < allOnLight->NumberOfOnTimes; i++)\r\n	{\r\n		if (DeltaTime >= " << (Options.bUseLowPrecisionTimes ? "((ulong)allOnLight->Times[i].Start * 100)" : "allOnLight->Times[i].Start") << " && DeltaTime < " << (Options.bUseLowPrecisionTimes ? "((ulong)allOnLight->Times[i].End * 100)" : "allOnLight->Times[i].End") << ")\r\n		{\r\n			#ifdef DEBUG\r\n			if (allOnLight->State == OFF) \r\n			{\r\n				Serial.println(\"Turning all lights ON.\");\r\n			}\r\n			allOnLight->State = ON;\r\n			#endif\r\n			TurnAllLights(ON);\r\n			return true;\r\n		}\r\n	}\r\n	if (bAllLightsOn)\r\n	{\r\n		#ifdef DEBUG\r\n		Serial.println(\"Turning all lights OFF.\");\r\n		allOnLight->State = OFF;	\r\n		#endif\r\n		TurnAllLights(OFF);\r\n	}\r\n	return false;\r\n}\r\n";
	OutputString << "\r\n";

	// All Lights Off
	OutputString << "/** Checks to see if all lights should be turned off or not. This function only sends ALL OFF command once per instance.\r\n *	- allOffLight: Light pointer containing all OnTimes for All Lights Off\r\n **/\r\nbool AllLightsOff(Light* allOffLight)\r\n{\r\n	for (int i = 0; i < allOffLight->NumberOfOnTimes; i++)\r\n	{\r\n		if (DeltaTime >= " << (Options.bUseLowPrecisionTimes ? "((ulong)allOffLight->Times[i].Start * 100)" : "allOffLight->Times[i].Start") << " && DeltaTime < " << (Options.bUseLowPrecisionTimes ? "((ulong)allOffLight->Times[i].End * 100)" : "allOffLight->Times[i].End") << ")\r\n		{\r\n			#ifdef DEBUG\r\n			Serial.println(\"Turning all lights OFF.\");\r\n			#endif\r\n			TurnAllLights(OFF);\r\n			allOffLight->Times[i].End = 0;\r\n			return true;\r\n		}\r\n	}\r\n	return false;\r\n}\r\n";
	OutputString << "\r\n";

	// Find Light
	OutputString << "/** Finds a light object in a given routine based on pin number.\r\n *	- routine: the routine to search through\r\n *	- pin: the pin of the light to find\r\n *	Returns: the light or NULL if no light found\r\n **/\r\nLight* FindLight(Routine* routine, int pin)\r\n{\r\n	for (int i=0; i < routine->NumberOfLights; i++)\r\n	{\r\n		if (routine->Lights[i]->Pin == pin)\r\n		{\r\n			#ifdef DEBUG\r\n			Serial.print(\"Found light with pin : \");\r\n			Serial.println(pin);\r\n			#endif\r\n			return routine->Lights[i];\r\n		}\r\n	}\r\n	#ifdef DEBUG\r\n	Serial.print(\"No light with pin \");\r\n	Serial.print(pin);\r\n	Serial.println(\" found...\");\r\n	#endif\r\n	return NULL;\r\n}\r\n";
	OutputString << "\r\n";
}

// -------- GENERATE SETUP FUNCTION -------- //
void CodeGenerator::GenerateSetupFunction()
{
	if (UseESP32) {
		// Forward web function declarations
		OutputString
			<< "void StartWebServer();\r\n"
			<< "void HandleWebRequests();\r\n"
			<< "String buildPage();\r\n"
			<< "String GetRoutineDescription(int routine);\r\n";
	}

	OutputString
		<< "void setup()\r\n"
		<< "{\r\n"
		<< "	Serial.begin(" << (UseESP32 ? "115200" : "9600") << ");\r\n";

	// Setup pins
	for (int i = 0; i < NUM_A_PINS; ++i)
	{
		// Skip A6 if legacy option enabled
		if (i == 5 && Options.bUseLegacyA6)
			continue;

		if (APinUsage[i])
			OutputString << "	pinMode(A" << i << ", " << (MotionSensePin == &(APinUsage[i]) ? "INPUT" : "OUTPUT") << ");\r\n";
	}

	for (int i = 0; i < NUM_D_PINS; ++i)
		if (DPinUsage[i])
			OutputString << "	pinMode(D" << (i + 2) << ", " << (MotionSensePin == &(DPinUsage[i]) ? "INPUT" : "OUTPUT") << ");\r\n";

	// Setup motion sensor pin
	if (!Options.motionSensorPin.empty())
		OutputString << "	pinMode(" << Options.motionSensorPin << ", INPUT_PULLUP);\r\n";

	// Setup skip button pin
	if (!Options.mp3SkipPin.empty())
		OutputString << "	pinMode(" << Options.mp3SkipPin << ", OUTPUT);\r\n";

	// Setup mp3 volume pin
	if (!Options.mp3VolumePin.empty())
		OutputString << "	pinMode(" << Options.mp3VolumePin << ", OUTPUT);\r\n";

	if (!Options.motorVoltagePin.empty())
		OutputString << "	pinMode(" << Options.motorVoltagePin << ", INPUT);\r\n";

	if (!Options.randomSeedPin.empty())
		OutputString << "	pinMode(" << Options.randomSeedPin << ", INPUT);\r\n";

	if (Options.bUseLegacyA6)
		OutputString << "	pinMode(A6, INPUT);\r\n";
	if (Options.bUseLegacyA7)
		OutputString << "	pinMode(A7, INPUT_PULLUP);\r\n";

	// In the event the option to "randomly select track" is checked, we'll need to initialize randomSeed
	if (Options.bRandomizeRoutineOrder && !Options.randomSeedPin.empty())
		OutputString << "	randomSeed(analogRead(" << Options.randomSeedPin << "));\r\n";

	OutputString << "	TurnAllLights(ON);\r\n";
	if (!Options.trainPinLeft.empty()) {
		const int maxTrainInit = Options.trainResetDuration <= 0 ? 20000 : Options.trainResetDuration;
		//if (Options.trainResetDuration <= 0) {
			// Auto-initialize train using motor avg
		OutputString
			<< (UseESP32 ? "	Serial.println(\"Change to actual file name.ino\");\r\n" : "")
			<< "    #ifdef DEBUG_TRAIN\r\n"
			<< "        Serial.println(\"Resetting train to designated side...\");\r\n"
			<< "    #endif\r\n"
			<< "    digitalWrite(" << Options.trainPinRight << ", OFF);      // Make sure train direction is L to R\r\n"
			<< "    delay(250);\r\n"
			<< "    digitalWrite(TrainPin, ON); // S/B A5\r\n"
			<< "    delay(10);\r\n"
			<< "    int startTime = millis();  // Record the start time\r\n"
			<< "    int maxDuration = " << maxTrainInit << ";\r\n"
			<< "    unsigned int time1;\r\n"
			<< "    unsigned int time2;\r\n"
			<< "    uint8_t motoravgmax = 0;\r\n"
			<< "    uint8_t motoravgmin = 255;\r\n"
			<< "    uint8_t motorAvg = 0;\r\n"
			<< "    uint8_t motorAvg1 = 0;\r\n";
		if (Options.bRandomizeRoutineOrder) {
			OutputString << "    for (int ii = 0; ii < NUM_ROUTINES; ii++) {  // resets used routine array to all false\r\n"
				<< "        used_routine[ii] = false;\r\n"
				<< "    }\r\n"
				<< "    all_used_up = false;        // reset all routine used variable\r\n";
		}
		OutputString
			<< "    while ((millis() - startTime < maxDuration) && (motorAvg <= motor_current_limit)) {\r\n"
			<< "        motorAvg = 0;\r\n"
			<< "        motorAvg1 = 0;\r\n"
			<< "        time1 = millis();\r\n"
			<< "		long sum = 0;\r\n"
			<< "		for (int i = 0; i < avg_count; i++) {\r\n"
			<< "			sum += analogRead(" << Options.motorVoltagePin << ");\r\n"
			<< "			delayMicroseconds(avgdly);\r\n"
			<< "		}\r\n"
			<< "		motorAvg = sum / avg_count;\r\n"
			<< "        time2 = millis() - time1;\r\n"
			<< "\r\n"
			<< "        #ifdef DEBUG_TRAIN\r\n"
			<< "            Serial.print(\"Time for averaging: \");\r\n"
			<< "            Serial.println(time2);\r\n"
			<< "            if (motorAvg >= motoravgmax)\r\n"
			<< "                motoravgmax = motorAvg;\r\n"
			<< "            if (motorAvg <= motoravgmin)\r\n"
			<< "                motoravgmin = motorAvg;\r\n"
			<< "            Serial.print(\"Max Average Current: \");\r\n"
			<< "            Serial.print(motoravgmax);\r\n"
			<< "            Serial.print(\", Min Average Current: \");\r\n"
			<< "            Serial.print(motoravgmin);\r\n"
			<< "            Serial.print(\", Average Motor Current: \");\r\n"
			<< "            Serial.println(motorAvg);\r\n"
			<< "        #endif\r\n"
			<< "    }\r\n"
			<< "\r\n"
			<< "    digitalWrite(TrainPin, OFF);\r\n"
			<< "    digitalWrite(" << Options.trainPinRight << ", ON);" << (Options.bPrettyPrint ? "	// Back train off the stop a bit\r\n" : "\r\n")
			<< "    delay(250);\r\n"
			<< "    digitalWrite(" << Options.trainPinRight << ", OFF);\r\n"
			<< "    while (millis() - startTime < maxDuration) {" << (Options.bPrettyPrint ? "	// Use this remaining train initialization time for all lights on at startup\r\n" : "\r\n")
			<< "        delay(100);\r\n"
			<< "    }\r\n"
			<< "    #ifdef DEBUG_TRAIN\r\n"
			<< "        Serial.println(\"Finished resetting train!\");\r\n"
			<< "    #endif\r\n";
	}
	else {
		// Ignore train setup - induce 7s delay
		OutputString << "	delay(7000);\r\n";
	}
	OutputString << "	TurnAllLights(OFF);\r\n";
	if (UseESP32)
		OutputString << "	StartWebServer();\r\n";
	OutputString
		<< "}\r\n"
		<< "\r\n";

}

// -------- GENERATE LOOP FUNCTION -------- //
void CodeGenerator::GenerateLoopFunction()
{
	OutputString
		<< "void loop()\r\n"
		<< "{\r\n";
	if (!Options.motionSensorPin.empty())
		OutputString << "	while (analogRead(PMotionSense) < 500) ;\r\n";
	if (Options.bUseHalloweenMP3Controls)
		OutputString << "	CurrentRoutine = 0;\r\n	digitalWrite(MP3SkipPin, HIGH);\r\n";
	else
		OutputString << "	CurrentRoutine = SkipToRoutine();\r\n";

	if (UseESP32) {
		if (Options.bPrettyPrint)
			OutputString
			<< (Options.bPrettyPrint ? "	// Handle new web requests\r\n" : "")
			<< "	HandleWebRequests();" << "\r\n";
	}

	// Commented out debug code
	OutputString
		<< "/* ---------------------------------------------------------------------------------------------------- */\r\n"
		<< "// This code is used only to force a specific routine to run.\r\n"
		<< "// 1 skip = Set 1, Routine 1\r\n"
		<< "    // CurrentRoutine = 1;\r\n"
		<< "    // int numberOfSkips = 1;\r\n"
		<< "    // for (int i = 0; i < numberOfSkips; i++)\r\n"
		<< "    // {\r\n"
		<< "    //     Serial.print(\" skipping...)\");\r\n"
		<< "    //     digitalWrite(MP3SkipPin, HIGH);\r\n"
		<< "    //     delay(150);\r\n"
		<< "    //     digitalWrite(MP3SkipPin, LOW);\r\n"
		<< "    //     delay(250);\r\n"
		<< "    // }\r\n"
		<< "    // Serial.println(\"Done skipping)\");\r\n"
		<< "/* ---------------------------------------------------------------------------------------------------- */\r\n";

	// Debug routine info
	OutputString
		<< "	#ifdef DEBUG_SKIP_ROUTINE\r\n"
		<< "		Serial.print(\"Current Routine: \");\r\n"
		<< "		Serial.println(CurrentRoutine);\r\n"
		<< "	#endif\r\n";

	OutputString
		<< "	StartTime = millis();\r\n"
		<< "	Light* allOffLight = FindLight(routines[CurrentRoutine], P_ALL_OFF);\r\n"
		<< "	Light* allOnLight = FindLight(routines[CurrentRoutine], P_ALL_ON);\r\n"
		<< "	do\r\n"
		<< "	{\r\n"
		<< "		DeltaTime = millis() - StartTime;\r\n";

	if (!Options.mp3VolumePin.empty())
		OutputString << "		digitalWrite(MP3VolumePin, DeltaTime < 7000 ? HIGH : LOW);\r\n";

	if (UseESP32) {
		OutputString << (Options.bPrettyPrint ? "		// This keeps web server responsive\r\n" : "")
			<< "		HandleWebRequests();" << "\r\n"
			<< "		if (WebSkipRequested) {" << "\r\n"
			<< "			WebSkipRequested = false;\r\n"
			<< "			break;" << (Options.bPrettyPrint ? "	// Break out of running routine immediately" : "") << "\r\n"
			<< "		}\r\n";
	}
	OutputString
		<< "		if (allOffLight != NULL)\r\n"
		<< "			AllLightsOff(allOffLight);\r\n"
		<< "		if (allOnLight != NULL)\r\n"
		<< "			bAllLightsOn = AllLightsOn(allOnLight);\r\n"
		<< "		for (int i=0; i < routines[CurrentRoutine]->NumberOfLights; i++)\r\n"
		<< "			CheckLight(routines[CurrentRoutine]->Lights[i]);\r\n\r\n";

	// Train train motor state
	if (!Options.trainPinLeft.empty()) {
		OutputString
			<< "		int pinStateL2R = digitalRead(" << Options.trainPinLeft << ");\r\n"
			<< "		int pinStateR2L = digitalRead(" << Options.trainPinRight << ");\r\n"
			<< "		if (pinStateR2L == HIGH || pinStateL2R == HIGH) {\r\n"
			<< "			if (pinStateL2R == HIGH)\r\n"
			<< "				motor_current_limit = motor_current_limit_L2R;\r\n"
			<< "			if (pinStateR2L == HIGH)\r\n"
			<< "				motor_current_limit = motor_current_limit_R2L;\r\n"
			<< "			delay(100);\r\n"
			<< "			uint8_t motorAvg = 0;\r\n"
			<< "			uint8_t motorAvg1 = 0;\r\n"
			<< "\r\n"
			<< "			long sum = 0;\r\n"
			<< "			for (int i = 0; i < avg_count; i++) {\r\n"
			<< "				sum += analogRead(" << Options.motorVoltagePin << ");\r\n"
			<< "				delayMicroseconds(avgdly);\r\n"
			<< "			}\r\n"
			<< "			motorAvg = sum / avg_count;\r\n"
			<< "			#ifdef DEBUG_TRAIN\r\n"
			<< "				Serial.print(\"Average Motor Current: \");\r\n"
			<< "				Serial.println(motorAvg);\r\n"
			<< "			#endif\r\n"
			<< "\r\n"
			<< "			if (motorAvg >= motor_current_limit) {\r\n"
			<< "				pinStateL2R = digitalRead(" << Options.trainPinLeft << ");\r\n"
			<< "				pinStateR2L = digitalRead(" << Options.trainPinRight << ");\r\n"
			<< "				if (pinStateL2R == HIGH) {\r\n"
			<< "					digitalWrite(" << Options.trainPinRight << ", OFF);\r\n"
			<< "					digitalWrite(" << Options.trainPinLeft << ", ON);\r\n"
			<< "					delay(250);\r\n"
			<< "					digitalWrite(" << Options.trainPinLeft << ", OFF);\r\n"
			<< "					#ifdef DEBUG_TRAIN\r\n"
			<< "						Serial.println(\"Motor stopped by current monitor. Code delay of 4 seconds has begun \");\r\n"
			<< "					#endif\r\n"
			<< "					delay(2000);\r\n"
			<< "				}\r\n"
			<< "				if (pinStateR2L == HIGH) {\r\n"
			<< "					digitalWrite(" << Options.trainPinLeft << ", OFF);\r\n"
			<< "					digitalWrite(" << Options.trainPinRight << ", ON);\r\n"
			<< "					delay(250);\r\n"
			<< "					digitalWrite(" << Options.trainPinRight << ", OFF);\r\n"
			<< "					#ifdef DEBUG_TRAIN\r\n"
			<< "						Serial.println(\"Motor stopped by current monitor. Code delay of 4 seconds has begun \");\r\n"
			<< "					#endif\r\n"
			<< "					delay(2000);\r\n"
			<< "				}\r\n"
			<< "			}\r\n"
			<< "		}\r\n";
	}

	OutputString
		<< "	} while (DeltaTime <= " << (Options.bUseLowPrecisionTimes ? "((ulong)routines[CurrentRoutine]->RoutineTime * 100)" : "routines[CurrentRoutine]->RoutineTime") << ");\r\n"
		<< "	for (int x=0; allOffLight != NULL && x < allOffLight->NumberOfOnTimes; x++)\r\n"
		<< "		allOffLight->Times[x].End = " << "routines[CurrentRoutine]->RoutineTime" << ";\r\n";
	if (Options.bUseHalloweenMP3Controls) {
		OutputString << "	digitalWrite(MP3SkipPin, LOW);\r\n";
	}

	OutputString << "}\r\n";
	OutputString << "\r\n";
}

// -------- GENERATE ESP32 WEB MODULE CODE -------- //
void CodeGenerator::GenerateESP32WebModuleCode()
{
	if (Options.bPrettyPrint) {
		OutputString 
			<< "// =============================================================\r\n"
			<< "//                   WEB INTERFACE MODULE\r\n"
			<< "//           (Self-contained & safe for auto-generated code)\r\n"
			<< "// =============================================================\r\n";
	}

	OutputString 
		<< "#include <WiFi.h>\r\n"
		<< "#include <WebServer.h>\r\n" << "\r\n"
		<< "WebServer server(80);\r\n" << "\r\n"
		<< "const char* WIFI_SSID = \"" << Options.wifiSSID << "\";\r\n"
		<< "const char* WIFI_PASS = \"" << Options.wifiPassword << "\";\r\n";
	if (Options.bPrettyPrint) {
		OutputString 
			<< "\r\n"
			<< "// ===========================================\r\n"
			<< "// Routine Descriptions For Dropdown\r\n"
			<< "// ===========================================\r\n";
	}
	OutputString
		<< "const char* RoutineDescriptions[] = {\r\n";
	for (std::list<std::string>::iterator itter = OrderTracker.routineNames.begin(); itter != OrderTracker.routineNames.end();) {
		OutputString << "    \"" << GetRoutineLabel(Routines.find(*itter)->second.routine);
		if (++itter == OrderTracker.routineNames.end())
			OutputString << "\"\r\n";
		else
			OutputString << "\",\r\n";
	}
	OutputString
		<< "};\r\n";
	if (Options.bPrettyPrint) {
		OutputString
			<< "// -------------------------------------------------------------\r\n"
			<< "//    Build Web Page (simple status + skip button)\r\n"
			<< "// -------------------------------------------------------------\r\n";
	}
	OutputString
		<< "String buildPage()\r\n"
		<< "{\r\n"
		<< "	String ip = WiFi.localIP().toString();\r\n";
	if (Options.bPrettyPrint) {
		OutputString
			<< "	// ================================================\r\n"
			<< "	// Build dynamic dropdown options\r\n"
			<< "	// ================================================\r\n";
	}
	OutputString
		<< "	String routineOptions = \"\";\r\n"
    	<< "	for (int i = 0; i < NUM_ROUTINES; i++) {\r\n"
    	<< "		routineOptions += \"<option value='\" + String(i) + \"'\";\r\n"
    	<< "		if (i == CurrentRoutine) routineOptions += \" selected\";\r\n"
    	<< "		routineOptions += \">\";\r\n"
    	<< "		routineOptions += String(i) + \" - \" + GetRoutineDescription(i);\r\n"
    	<< "		routineOptions += \"</option>\";\r\n"
    	<< "	}\r\n";
	if (Options.bPrettyPrint) {
		OutputString
		<< "	// ================================================\r\n"
    	<< "	// Begin HTML\r\n"
    	<< "	// ================================================\r\n";
	}
	OutputString
		<< "	String html = R\"(\r\n";
	// Start html content
	OutputString
		<< "<!DOCTYPE html>\r\n"
		<< "<html>\r\n"
		<< "<head>\r\n"
		<< "<meta charset='utf-8'>\r\n"
		<< "<meta name='viewport' content='width=device-width, initial-scale=1'>\r\n"
		<< "<title>Christmas Scene Controller</title>\r\n"
		<< "\r\n"
		<< "<style>\r\n"
		<< "    :root {\r\n"
		<< "        --bg: #0d1117;\r\n"
		<< "        --card: #161b22;\r\n"
		<< "        --text: #e6edf3;\r\n"
		<< "        --accent: #2ea043;\r\n"
		<< "        --accent2: #238636;\r\n"
		<< "        --btn-text: white;\r\n"
		<< "        --font: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    body {\r\n"
		<< "        margin: 0;\r\n"
		<< "        padding: 0;\r\n"
		<< "        background: var(--bg);\r\n"
		<< "        color: var(--text);\r\n"
		<< "        font-family: var(--font);\r\n"
		<< "        text-align: center;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    .container {\r\n"
		<< "        padding: 18px;\r\n"
		<< "        max-width: 480px;\r\n"
		<< "        margin: auto;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    /* Sticky header when scrolling */\r\n"
		<< "    .status-card {\r\n"
		<< "        background: var(--card);\r\n"
		<< "        border-radius: 14px;\r\n"
		<< "        padding: 20px;\r\n"
		<< "        box-shadow: 0 0 10px #000a;\r\n"
		<< "        margin-bottom: 90px;\r\n"
		<< "        position: sticky;\r\n"
		<< "        top: 0;\r\n"
		<< "        z-index: 10;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    .routine-number {\r\n"
		<< "        font-size: 3.2rem;\r\n"
		<< "        font-weight: bold;\r\n"
		<< "        margin: 10px 0;\r\n"
		<< "        text-shadow: 0 0 8px rgba(46,160,67,0.5);\r\n"
		<< "        transition: opacity 0.3s ease;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    #desc {\r\n"
		<< "        transition: opacity 0.3s ease;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    .ip {\r\n"
		<< "        font-size: 0.9rem;\r\n"
		<< "        opacity: 0.7;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    /* Floating bottom bar */\r\n"
		<< "    .skip-bar {\r\n"
		<< "        position: fixed;\r\n"
		<< "        bottom: 0;\r\n"
		<< "        left: 0;\r\n"
		<< "        right: 0;\r\n"
		<< "        padding: 18px;\r\n"
		<< "        padding-bottom: env(safe-area-inset-bottom);\r\n"
		<< "        background: var(--card);\r\n"
		<< "        box-shadow: 0 -2px 10px #000a;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    button {\r\n"
		<< "        width: 92%;\r\n"
		<< "        max-width: 360px;\r\n"
		<< "        font-size: 1.35rem;\r\n"
		<< "        padding: 16px;\r\n"
		<< "        border-radius: 14px;\r\n"
		<< "        border: none;\r\n"
		<< "        background: var(--accent);\r\n"
		<< "        color: var(--btn-text);\r\n"
		<< "        font-weight: bold;\r\n"
		<< "        margin-top: 6px;\r\n"
		<< "        transition: transform 0.1s, background 0.15s;\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    button:active {\r\n"
		<< "        transform: scale(0.97);\r\n"
		<< "        background: var(--accent2);\r\n"
		<< "    }\r\n"
		<< "\r\n"
		<< "    /* Mobile-optimized dropdown */\r\n"
		<< "    select {\r\n"
		<< "        width: 92%;\r\n"
		<< "        padding: 16px;\r\n"
		<< "        border-radius: 12px;\r\n"
		<< "        border: none;\r\n"
		<< "        font-size: 1.15rem;\r\n"
		<< "        margin-bottom: 14px;\r\n"
		<< "        background: #0f1722;\r\n"
		<< "        color: white;\r\n"
		<< "        appearance: none;\r\n"
		<< "        background-image: url(\"data:image/svg+xml;utf8,<svg fill='white' height='20' width='20' viewBox='0 0 20 20' xmlns='http://www.w3.org/2000/svg'><polygon points='0,0 20,0 10,12'/></svg>\");\r\n"
		<< "        background-position: right 14px center;\r\n"
		<< "        background-repeat: no-repeat;\r\n"
		<< "    }\r\n"
		<< "</style>\r\n"
		<< "\r\n"
		<< "<script>\r\n"
		<< "    // Smooth update every 2 seconds\r\n"
		<< "    setInterval(() => {\r\n"
		<< "\r\n"
		<< "        fetch('/routine')\r\n"
		<< "            .then(r => r.text())\r\n"
		<< "            .then(num => {\r\n"
		<< "                let el = document.getElementById('routine');\r\n"
		<< "                el.style.opacity = 0;\r\n"
		<< "                setTimeout(() => {\r\n"
		<< "                    el.innerText = num;\r\n"
		<< "                    el.style.opacity = 1;\r\n"
		<< "                }, 150);\r\n"
		<< "            });\r\n"
		<< "\r\n"
		<< "        fetch('/description')\r\n"
		<< "            .then(r => r.text())\r\n"
		<< "            .then(desc => {\r\n"
		<< "                let de = document.getElementById('desc');\r\n"
		<< "                de.style.opacity = 0;\r\n"
		<< "                setTimeout(() => {\r\n"
		<< "                    de.innerText = desc;\r\n"
		<< "                    de.style.opacity = 1;\r\n"
		<< "                }, 150);\r\n"
		<< "            });\r\n"
		<< "\r\n"
		<< "    }, 2000);\r\n"
		<< "</script>\r\n"
		<< "\r\n"
		<< "</head>\r\n"
		<< "<body>\r\n"
		<< "\r\n"
		<< "<div class='container'>\r\n"
		<< "    <h1>&#127876; Christmas Scene Controller &#127926;</h1>\r\n"
		<< "\r\n"
		<< "    <div class='status-card'>\r\n"
		<< "        <div>Current Routine:</div>\r\n"
		<< "\r\n"
		<< "        <div id='routine' class='routine-number'>)\";\r\n"
		<< "\r\n"
		<< "    html += String(CurrentRoutine);\r\n"
		<< "\r\n"
		<< "    html += R\"(</div>\r\n"
		<< "\r\n"
		<< "        <div id='desc' style=\"margin-top:10px; font-size:1.2rem; opacity:1;\">)\";\r\n"
		<< "\r\n"
		<< "    html += GetRoutineDescription(CurrentRoutine);\r\n"
		<< "\r\n"
		<< "    html += R\"(</div>\r\n"
		<< "\r\n"
		<< "        <div class='ip'>Device IP: )\";\r\n"
		<< "    html += ip;\r\n"
		<< "    html += R\"(</div>\r\n"
		<< "    </div>\r\n"
		<< "</div>\r\n"
		<< "\r\n"
		<< "<div class='skip-bar'>\r\n"
		<< "\r\n"
		<< "    <form action='/skip' method='POST'>\r\n"
		<< "        <button type='submit'>Skip to Next Routine &#10148;</button>\r\n"
		<< "    </form>\r\n"
		<< "\r\n"
		<< "    <form action='/skipto' method='POST'>\r\n"
		<< "        <select name=\"routine\">)\";\r\n"
		<< "\r\n"
		<< "    html += routineOptions;\r\n"
		<< "\r\n"
		<< "    html += R\"(</select>\r\n"
		<< "        <button type='submit'>Skip to Routine</button>\r\n"
		<< "    </form>\r\n"
		<< "\r\n"
		<< "</div>\r\n"
		<< "\r\n"
		<< "</body>\r\n"
		<< "</html>\r\n"
		<< ")\";\r\n"
		<< "\r\n"
		<< "    return html;\r\n"
		<< "}\r\n";

	// End html content

	if (Options.bPrettyPrint) {
		OutputString
			<< "\r\n"
			<< "// -------------------------------------------------------------\r\n"
			<< "//                Web Request Handlers\r\n"
			<< "// -------------------------------------------------------------\r\n"
			<< "// Simple endpoint returning only the current routine number\r\n";
	}

	OutputString
		<< "void RequestWebSkip() {\r\n"
		<< "	 WebSkipRequested = true;\r\n"
		<< "}\r\n"
		<< "\r\n"
		<< "void handleRoutine() {\r\n"
		<< "	server.send(200, \"text/plain\", String(CurrentRoutine));\r\n"
		<< "}\r\n"
		<< "\r\n"
		<< "void handleRoot() {\r\n"
		<< "	server.send(200, \"text/html\", buildPage());\r\n"
		<< "}\r\n"
		<< "\r\n"
		<< "void handleSkip() {\r\n"
		<< "	RequestWebSkip();   // set flag (non-blocking)\r\n"
		<< "	server.sendHeader(\"Location\", \"/\");\r\n"
		<< "	server.send(303);    // redirect back to main page\r\n"
		<< "}\r\n"
		<< "\r\n"
		<< "String GetRoutineDescription(int routine) {\r\n"
		<< "	int count = sizeof(RoutineDescriptions) / sizeof(RoutineDescriptions[0]);\r\n"
		<< "	return routine < 0 || routine >= count\r\n"
		<< "		? \"Unknown Routine\"\r\n"
		<< "		: RoutineDescriptions[routine];\r\n"
		<< "}\r\n";

	if (Options.bPrettyPrint) {
		OutputString
			<< "\r\n"
			<< "// -------------------------------------------------------------\r\n"
			<< "//            Start Web Server (call from setup())\r\n"
			<< "// -------------------------------------------------------------\r\n";
	}
	OutputString
		<< "void StartWebServer() {\r\n"
		<< "	delay(500);\r\n"
		<< "\r\n"
		<< "	server.on(\"/routine\", handleRoutine);\r\n"
		<< "\r\n"
		<< "	Serial.println(\"Starting WiFi...\");\r\n"
		<< "	WiFi.mode(WIFI_STA);\r\n"
		<< "	WiFi.begin(WIFI_SSID, WIFI_PASS);\r\n"
		<< "\r\n"
		<< "	while (WiFi.status() != WL_CONNECTED) {\r\n"
		<< "		delay(300);\r\n"
		<< "		Serial.print(\".\");\r\n"
		<< "	}\r\n"
		<< "	Serial.println(\"WiFi Connected.\");\r\n"
		<< "	Serial.print(\"IP Address: \");\r\n"
		<< "	Serial.println(WiFi.localIP());\r\n"
		<< "	Serial.print(\"MAC: \");\r\n"
		<< "	Serial.println(WiFi.macAddress());\r\n"
		<< "\r\n"
		<< "	// Routes\r\n"
		<< "	server.on(\"/\", handleRoot);\r\n"
		<< "	server.on(\"/skip\", HTTP_POST, handleSkip);\r\n"
    	<< "	server.on(\"/routine\", HTTP_GET, []() {\r\n"
    	<< "	    server.send(200, \"text/plain\", String(CurrentRoutine));\r\n"
    	<< "	});\r\n"
		<< "\r\n"
		<< "	server.on(\"/description\", HTTP_GET, []() {\r\n"
		<< "		server.send(200, \"text/plain\", GetRoutineDescription(CurrentRoutine));\r\n"
		<< "	});\r\n"
		<< "\r\n"
		<< "server.on(\"/skipto\", HTTP_POST, []() {\r\n"
		<< "    if (server.hasArg(\"routine\")) {\r\n"
		<< "        int r = server.arg(\"routine\").toInt();\r\n"
		<< "        // clamp to valid range\r\n"
		<< "        if (r < 0) r = 0;\r\n"
		<< "        if (r >= NUM_ROUTINES) r = NUM_ROUTINES - 1;\r\n"
		<< "        ForcedNextRoutine = r;      // set override\r\n"
		<< "        WebSkipRequested  = true;   // exit current routine\r\n"
		<< "        // *** FIX: Redirect back to main page ***\r\n"
		<< "        server.sendHeader(\"Location\", \"/\");\r\n"
		<< "        server.send(303);\r\n"
		<< "    } else {\r\n"
		<< "        server.send(400, \"text/plain\", \"Missing routine number\");\r\n"
		<< "    }\r\n"
		<< "});\r\n"
		<< "\r\n"
		<< "	server.begin();\r\n"
		<< "	Serial.println(\"Web Server started.\");\r\n"
		<< "}\r\n";

	if (Options.bPrettyPrint) {
		OutputString
			<< "\r\n"
			<< "// -------------------------------------------------------------\r\n"
			<< "//      Call server.handleClient() inside loop()\r\n"
			<< "// -------------------------------------------------------------\r\n";
	}
	OutputString
		<< "void HandleWebRequests() {\r\n"
		<< "	server.handleClient();\r\n"
		<< "}\r\n";
}

std::string CodeGenerator::GenerateCode()
{
	OutputString.clear();

	GenerateDefines();
	GeneratePinSetup();
	if (Options.bPrettyPrint)
	{
		OutputString << "/* ----------------------------------------------------------------------------------------------------\r\n";
		OutputString << " *                                           Data Structures                                           \r\n";
		OutputString << " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}
	GenerateDataStructures();
	if (Options.bPrettyPrint)
	{
		OutputString << "/* ----------------------------------------------------------------------------------------------------\r\n";
		OutputString << " *                                       Light/Routine Variables                                       \r\n";
		OutputString << " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}
	GenerateLightAndRoutineVariables();
	if (Options.bPrettyPrint)
	{
		OutputString 
			<< "/* ----------------------------------------------------------------------------------------------------\r\n"
			<< " *                                         Town Scene Functions                                        \r\n"
			<< " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}
	GenerateCheckLightFunction();
	GenerateSkipToRoutineFunction();
	GenerateLightFunctions();
	if (Options.bPrettyPrint)
	{
		OutputString << "/* ----------------------------------------------------------------------------------------------------\r\n";
		OutputString << " *                                          Arduino Functions                                          \r\n";
		OutputString << " * ---------------------------------------------------------------------------------------------------- */\r\n";
	}
	GenerateSetupFunction();
	GenerateLoopFunction();
	if (UseESP32)
	{
		GenerateESP32WebModuleCode();
	}


	return OutputString.str();
}