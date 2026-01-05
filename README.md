# Town Scene Code Generator

A Windows desktop application for generating Arduino code to control lights synchronized with music for a town scene display. The application reads WAV files with embedded CUE point markers, allows you to configure light routines, and generates ESP32-compatible Arduino code.

## Features

- **Drag & Drop WAV Files**: Easily add WAV files with CUE point markers
- **Routine Configuration**: Create and manage light routines with building/light assignments
- **Arduino Code Generation**: Automatically generate ESP32-compatible Arduino code
- **MP3 Player File Transfer**: Copy WAV files to MP3 player via drive letter selection
- **Project Management**: Save and load routine projects

## Requirements

- Windows 10/11
- Python 3.11+ (for development)
- PyQt6
- PyInstaller (for building executable)

## Installation

### From Source

1. Clone or download this repository
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
3. Run the application:
   ```bash
   python src/main.py
   ```

### Building Executable

To create a standalone Windows executable:

```bash
python build_exe.py
```

The executable will be created in the `dist` folder as `TownSceneGenerator.exe`.

## Usage

### 1. Add WAV Files

- Drag and drop WAV files into the left panel, or
- Click "Add Files..." to browse for WAV files
- The application will automatically parse CUE point markers from the files

### 2. Create a Routine

1. Click "Routine" → "New Routine" from the menu
2. Enter a routine name
3. Select a WAV file from the dropdown
4. Configure buildings and lights:
   - Click "Add Building" to add a new building
   - Enter building name, ID, and GPIO pin numbers (comma-separated)
5. Click "Save Routine" to save your configuration

### 3. Generate Arduino Code

1. Configure your routines
2. Go to "File" → "Export Arduino Code..."
3. Choose a location to save the `.ino` file
4. Open the generated file in Arduino IDE and upload to your ESP32

### 4. Transfer Files to MP3 Player

1. Connect your MP3 player to the computer
2. In the right panel, select the drive letter where the MP3 player is mounted
3. Select which files to transfer (checkboxes)
4. Optionally specify a destination folder
5. Click "Transfer Files" to copy WAV files to the MP3 player

## Project Structure

```
Town-Scene-Code-Generator/
├── src/
│   ├── main.py              # Application entry point
│   ├── gui/                 # GUI components
│   │   ├── main_window.py
│   │   ├── file_manager.py
│   │   ├── routine_editor.py
│   │   └── mp3_transfer.py
│   ├── core/                # Core functionality
│   │   ├── wav_parser.py
│   │   ├── routine_manager.py
│   │   ├── code_generator.py
│   │   └── mp3_file_manager.py
│   └── templates/
│       └── arduino_template.ino
├── build_exe.py            # PyInstaller build script
├── requirements.txt        # Python dependencies
└── README.md
```

## WAV File Format

The application expects WAV files with embedded CUE point markers in the standard RIFF format:
- CUE chunk: Contains marker positions (sample offsets)
- LIST chunk with LABL subchunks: Contains marker labels
- LIST chunk with NOTE subchunks: Contains marker descriptions (optional)

## Arduino Code

The generated Arduino code is designed for ESP32 and includes:
- Pin definitions for lights
- MP3 player control (DFPlayer Mini)
- Marker-based timing logic
- Non-blocking light control using `millis()`

### MP3 Player Configuration

By default, the code uses:
- RX Pin: 16
- TX Pin: 17

You can change these in the application via "Settings" → "MP3 Player Configuration..."

## License

This project is provided as-is for personal use.

## Troubleshooting

### WAV files not parsing
- Ensure your WAV files contain CUE point markers
- Check that the file is a valid WAV format (RIFF WAVE)

### MP3 player not detected
- Ensure the MP3 player is connected and mounted as a drive
- Click "Refresh" in the MP3 transfer panel
- Verify the drive letter is correct

### Arduino code generation fails
- Ensure at least one routine is configured
- Check that routines have valid WAV files and light configurations

