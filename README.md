# GuitarBuddy

Guitar Buddy is a digital instructor for guitar lessons. The project is being built as the capstone project for ECE 445: Senior Design, Fall 2018. 

## Getting Started

The software portion of Guitar Buddy consists of two programs: one for conversion from MIDI to LED array assignments in a dense track structure, and one to actually transmit the dense track structure to the microcontroller as a sparse, "real-time" track structure. These can be found under the Bluetooth_Transmission and MIDI_to_Binary_Conversion directories.

### Prerequisites

Requires:
-- g++ compiiler to make the C++ program in the MIDI conversion program
-- Arduino IDE and Python 3.X for bluetooth transmission
-- MuseScore2 from Windows store to investigate MIDI channels

## Running the tests

To run the MIDI conversion program, follow these instructions in the MIDI_to_Binary_Conversion directory:

```
make "cleanwin or cleanlinux"
make
./MIDIToBytes songname channel_num
```

Example:
```
make cleanlinux
make
./MIDIToBytes JingleBells 1
```

## Authors

* **Austin Born** - [Batteryman212](https://github.com/Batteryman212)
* **Chris Horn** - [Gadget212](https://github.com/Gadget212)


## Acknowledgments

* Evandro Copercini for Bluetooth serial communciations for ESP32
* MIDI format help from https://www.csie.ntu.edu.tw/~r92092/ref/midi/
* MIDI http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html
