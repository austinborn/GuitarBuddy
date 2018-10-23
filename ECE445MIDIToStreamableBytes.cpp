// ECE 445 Senior Design - MIDI to Streamable byte format
/*
Format:
MThd <length of header data>
<header data>
MTrk <length of track data>
<track data>
...

<Header Chunk> = <type><length><format><ntrks><division>

<Track Chunk> = <type><length><MTrk event>+...

<MTrk event> = <delta-time><event>

<event> = <MIDI event> | <sysex event> | <meta-event>
*/

#include <iostream>
#include <fstream>
#include <chrono>
//#include <cstring>
//#include <cstddef>

using namespace std;

#define     BYTE_SIZE       8

int main(){
    //Initializations
    int next_buffer_size = 4;
    char byte_buffer [4];

    //Open file
    ifstream infile(".\SayItAintSo.mid");

    //Get file length
    infile.seekg(0, infile.end);
    int file_length = infile.tellg();
    infile.seekg(0, infile.beg);

    int bytes_read = 0;
    while(bytes_read < file_length){
        for(int i = 0; i < next_buffer_size; i++){
            infile.read(&(byte_buffer[i]), BYTE_SIZE);
        }

        for(int j = 0; j < next_buffer_size; j++){
            cout << byte_buffer[j] << " ";
        }
        cout << endl;

        next_buffer_size = 1;
        _sleep(1000);
    }

    return 0;
}

//Check if MThd or MTrk - first 32 bits
//If MThd, do head stuff
//If MTrk, do track stuff

//If head:
//First 32 bits: length, aka number of bytes following (6)

//Next 16 bits: format (0, 1, or 2)
//  0 - a single, multi-channel track
//  1 - one or more simultaneous tracks
//  2 - one or more sequentially independent single tracks

//Next 16 bits: number of tracks

//Next 16 bits: division
//If bit 15 = 0: B[14:0] = Number of ticks making up a quarter note
//If bit 15 = 1: Delta times correspond to subdivisions of a second:
//  B[14:8] = either -24, -25, -29, or -30: standard time code formats for fps
//      -29 = 30 drop frame
//  B[7:0] = resolution within a frame, typically 4, 8, 10, 80, or 100


//If track:
//First 32 bits: length, aka number of bytes following

//While there are bits remaining:
// First X bits: delta-time (amount of time to pass before the following event)
//



// Sparse coding of LED array assignments