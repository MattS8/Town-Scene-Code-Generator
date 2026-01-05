// Arduino Code for ESP32 Town Scene Light Controller
// This code controls lights synchronized with music played on an MP3 player
// MP3 player control is handled by the ESP32

#include <SoftwareSerial.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// MP3 Player Configuration (DFPlayer Mini)
#define MP3_RX_PIN {{MP3_RX_PIN}}  // ESP32 RX pin connected to DFPlayer TX
#define MP3_TX_PIN {{MP3_TX_PIN}}  // ESP32 TX pin connected to DFPlayer RX

// Pin Definitions for Lights
// Generated pins: {{ALL_PINS}}
{{PIN_DEFINITIONS}}

// ============================================================================
// ROUTINE DATA
// ============================================================================

// Routine data structure
struct Marker {
  unsigned long position_ms;
  String label;
  int id;
};

struct Routine {
  String name;
  String wav_file;
  int num_markers;
  Marker* markers;
  int* pins;
  int num_pins;
};

// Routine data (populated from {{ROUTINES_JSON}})
// This will be replaced during code generation
const int NUM_ROUTINES = {{NUM_ROUTINES}};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

SoftwareSerial mp3Serial(MP3_RX_PIN, MP3_TX_PIN);

int current_routine = 0;
unsigned long routine_start_time = 0;
bool routine_playing = false;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  mp3Serial.begin(9600);
  
  // Initialize all light pins as OUTPUT
  // TODO: Initialize pins based on your configuration
  // Example: pinMode(PIN_2, OUTPUT);
  
  // Initialize MP3 player
  delay(1000);
  // Send initialization commands to MP3 player if needed
  
  Serial.println("Town Scene Controller Ready");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Check for routine start/control commands
  // This can be triggered by button press, serial command, etc.
  
  if (routine_playing) {
    update_lights();
  }
  
  // Handle serial commands for routine selection
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("play:")) {
      int routine_num = command.substring(5).toInt();
      start_routine(routine_num);
    } else if (command == "stop") {
      stop_routine();
    }
  }
}

// ============================================================================
// ROUTINE CONTROL
// ============================================================================

void start_routine(int routine_index) {
  if (routine_index < 0 || routine_index >= NUM_ROUTINES) {
    Serial.println("Invalid routine index");
    return;
  }
  
  current_routine = routine_index;
  routine_start_time = millis();
  routine_playing = true;
  
  // Start MP3 playback
  // TODO: Send command to MP3 player to play the file
  // Example for DFPlayer: sendCommand(0x0F, routine_data[routine_index].wav_file_number);
  
  Serial.print("Started routine: ");
  Serial.println(routine_data[routine_index].name);
}

void stop_routine() {
  routine_playing = false;
  
  // Stop MP3 playback
  // TODO: Send stop command to MP3 player
  
  // Turn off all lights
  turn_off_all_lights();
  
  Serial.println("Routine stopped");
}

// ============================================================================
// LIGHT CONTROL
// ============================================================================

void update_lights() {
  if (!routine_playing) return;
  
  unsigned long current_time = millis() - routine_start_time;
  
  // Get current routine data
  Routine* routine = &routine_data[current_routine];
  
  // Check each marker in the routine
  for (int i = 0; i < routine->num_markers; i++) {
    Marker* marker = &routine->markers[i];
    
    // Check if marker time has been reached
    if (current_time >= marker->position_ms) {
      // Execute light actions for this marker
      execute_marker_actions(marker);
    }
  }
}

void execute_marker_actions(Marker* marker) {
  // TODO: Implement marker-based light control
  // This should map marker labels/IDs to specific light actions
  // Example:
  // if (marker->label == "Building1_On") {
  //   digitalWrite(PIN_2, HIGH);
  // } else if (marker->label == "Building1_Off") {
  //   digitalWrite(PIN_2, LOW);
  // }
}

void turn_off_all_lights() {
  // TODO: Turn off all lights
  // Example: for each pin, digitalWrite(pin, LOW);
}

// ============================================================================
// MP3 PLAYER CONTROL
// ============================================================================

void sendMP3Command(byte command, byte data1, byte data2) {
  // DFPlayer Mini command format
  byte buffer[10] = {0x7E, 0xFF, 0x06, command, 0x00, data1, data2, 0x00, 0x00, 0xEF};
  mp3Serial.write(buffer, 10);
}

void playTrack(int track_number) {
  sendMP3Command(0x03, (track_number >> 8) & 0xFF, track_number & 0xFF);
}

void pausePlayback() {
  sendMP3Command(0x0E, 0x00, 0x00);
}

void stopPlayback() {
  sendMP3Command(0x16, 0x00, 0x00);
}

// ============================================================================
// NOTE: This template will be populated with actual routine data during
// code generation. The placeholders {{ROUTINES_JSON}}, {{ALL_PINS}}, etc.
// will be replaced with actual values.
// ============================================================================

