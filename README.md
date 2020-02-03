# Town Scene Code Generator
This application automatically generates Arduino code for a Town Scene project. It works by parsing `.wav` files, looking for cue markers with specially formatted tags, and creating "routines" to control town scenes. These routines are used to generate code which will tell the Arduino how and when to cycle through the songs on an MP3 player as well as when to turn light pins on and off.

## Hardware Used
This program was created for use with an **Arduino Nano**. It may work for other Arduino devices, however your mileage may vary. It can also interface with any MP3 player that supports file transfer via the Windows file explorer.

## Usage
### Declaring Used Pins
First, you must enter the pin numbers used to control the MP3 skip and MP3 volume control. These must be entered as standard pins (i.e. `D1` or `A2`).

Second, check all the boxes next to pins used to control the lights in each house. **Do not check the boxes for pins that are used for MP3 skip, volume control, train pins, etc.**.

* **Important:** If your hardware turns lights on by sending a LOW value, be sure to check the `Swap On/Off Values` checkbox.

### Creating Proper Audio Files
The program reads cue markers from `.wav` files in order to determine when to turn light pins on and off. This means that you can mixdown multiple songs into one big audio file (called a *routine*). 

#### Adding Cue Markers
Add cue markers where ever you want to turn a light on, and set the duration to whenever you want those lights to turn off. For example, if you add cue markers to the beginning of each song and make the cue span the duration equal of each respective song, you will effectively get lights that turn on and off with each and every song.

#### Cue Marker Naming Convention
Cue marker names must follow a strict naming convention in order for the program to understand which lights must be turned on and off. 

Put the name of the house (something unique) followed by a space and then the pin number for that house. For example: `SomeLight A2`. Make sure to use the same name whenever you reference that house multiple times within the same scene.

Names must not be the same as a **Special Tag** (see below) nor contain spaces or special characters (such as `!,".'\][` etc...).

##### Special Tags
The following are special names you can give to cue markers to denote certain functions:
* `ALL_ON` - Turns all light pins on (excluding train pins)
* `ALL_OFF` - Turns all light pins off (excluding train pins)

##### Tips
- Add some additional time to the end of each routine. This will ensure that the entire routine will run to completion. The amount of time to add depends on the inaccuracy of your MP3 timing.

### Importing Audio Files

The preferred method of importing is to simply drag the `.wav` file onto the program. You can also manually add extracted cue info by clicking the `Add Routine` button. However, this will make it impossible to automatically load the song files to the MP3 player.

If you have an MP3 player connected, you can automatically send audio files by clicking either the `Send Code to Arduino` or `Upload Code` button. 
> **NOTE:** You must save an MP3 version of your audio file with the same name and at the same file location of the corresponding `.wav` file.

### Sending Code Directly to Arduino IDE/Device
In order to send code directly to the Arduino IDE and/or device, you must have the Arduino IDE installed and set up on your computer. Visit the [Arduino website](https://www.arduino.cc/en/guide/windows) to set that up. Open the IDE and ensure the board type is set to `Arduino Nano` and the COM Port is properly set up.

You mus also set the PATH environment in Windows to recognize Arduino commands from the command line. Find the folder location that contains `arduino.exe` and add that path to your system PATH environment (For help on how to do that, check [this guide](https://www.computerhope.com/issues/ch000549.htm)). 

> **NOTE:** If you installed the Windows App version of Arduino IDE, it is probably in a hidden folder under `C:/Program Files/WindowsApps/ArduinoLLC...`. 

Once your environment variable is good to go, you can open the IDE directly by pressing the `Send Code to Arduino` button. You can upload code directly to a connected Arduino device by clicking the `Upload Code Directly` button.


## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.
