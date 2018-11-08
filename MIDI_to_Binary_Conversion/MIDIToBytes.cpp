// ECE 445 Senior Design - MIDI to Streamable byte format

/*
To Run in GuitarBuddy directory:
make clean
make
./MIDIToBytes "songname.mid" "songname.csv"
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

#include <unistd.h>
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <ctime>
#include "MIDIToBytes.h"

using namespace std;

//Initialize global structures
string notes [12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
string octaves [11] = {"-1", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

const int BUFFER_SIZE = 4;

int main(int argc, char** argv){

    //Initializations
    char buf [BUFFER_SIZE];
    int i = 0;
    clock_t begin = clock();

    //Open MIDI file
    fstream infile;
    infile.open("MIDI_Files/" + string(argv[1]), fstream::in);

    //Create output file
    fstream outfile;
    outfile.open("CSV_Files/" + string(argv[2]), fstream::in | fstream::out | fstream::trunc);

    //Get MIDI file length
    infile.seekg(0, infile.end);
    long file_length = infile.tellg();
    long bytes_left = file_length;
    infile.seekg(0, infile.beg);

    //Get "MThd", but do nothing with it
    readFromFile(infile, buf, 4, bytes_left);

    //Get length of Header file
    readFromFile(infile, buf, 4, bytes_left);
    long length = buf[0];
    for (i = 0; i < 4; i++)
        length = (length << 8) + (unsigned char)buf[i];

    //Get format of Header file
    readFromFile(infile, buf, 2, bytes_left);
    short format;
    for (i = 0; i < 2; i++)
        format = (format << 8) + (unsigned char)buf[i];
    outfile << "File format - " << (unsigned int)format << endl;

    //Get number of tracks
    readFromFile(infile, buf, 2, bytes_left);
    short ntrks;
    for (i = 0; i < 2; i++)
        ntrks = (ntrks << 8) + (unsigned char)buf[i];
    outfile << "# of Tracks - " << ntrks << endl;
    

    //Get time division
    readFromFile(infile, buf, 2, bytes_left);
    short division;
    for (i = 0; i < 2; i++)
        division = (division << 8) + (unsigned char)buf[i];

    //If division bit 15 = 1, divison[14:0] is ticks per quarter note
    if(division & 0x80){
        long tpq = division & 0x7F;
        outfile << "Ticks/quarter note - " << tpq << endl;
    }
    //Else division[14:8] is negative fps, division[7:0] is ticks per frame
    else{
        long fps = -(int)(division & 0x70);
        long tpf = division & 0x0F;
        outfile << "Frames/second - " << fps << endl;
        outfile << "Ticks/frame - " << tpf << endl;
    }

    //Initialize track variables
    long trk_length;
    long trk_bytes_left;
    long long total_time;
    int track_num = 0;

    //Loop through each track
    while(bytes_left > 0){

        //Increment track number
        track_num += 1;
        outfile << endl << "Start of track " <<track_num << " at Byte " << file_length - bytes_left << endl;

        //Get "MTrk" but do nothing with it
        readFromFile(infile, buf, 4, bytes_left);

        //Get length of track
        trk_length = 0;
        readFromFile(infile, buf, 4, bytes_left);
        for (int i = 0; i < 4; i++)
            trk_length = (trk_length << 8) + (buf[i] & 0x00FF);
        outfile << "Track length: " << trk_length << endl;

        //Initialize MIDI Event variables
        std::queue<long> delta_time_q;
        bool vlq_left;
        long long delta_time = 0;
        long vlq_byte;

        //Loop through each MIDI Event
        trk_bytes_left = trk_length;
        total_time = 0;

        while(trk_bytes_left > 0){

            //Get delta time of MIDI event
            vlq_left = true;
            
            //Push variable length quantity to queue
            while(vlq_left){
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                delta_time_q.push(buf[0]);
                if (!(buf[0] & 0x80))
                    vlq_left = false;
            }

            //Initialize delta_time to 0
            delta_time = 0;

            //For byte in variable length quantity, add to total delta_time value
            while(!delta_time_q.empty()){
                vlq_byte = delta_time_q.front();
                delta_time_q.pop();
                delta_time = (delta_time << 7) | (vlq_byte & 0x7F);
            }

            total_time += delta_time;

            //Get status byte for MIDI event
            readFromFile(infile, buf, 1, bytes_left);
            trk_bytes_left -= 1;
            unsigned char status = buf[0];

            //Parse Status
            if (status == 0x80) { //Note off event

                //Get Note number (and skip velocity)
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                long note_num = buf[0];

                //Record in CSV
                outfile << total_time << ": " << "Off, " << noteFinder(note_num) << endl;
            }

            else if (status == 0x90) { //Note on event

                //Get Note number (and skip velocity)
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                long note_num = buf[0];

                //Record in CSV
                outfile << total_time << ": " << "On, " << noteFinder(note_num) << endl;
            }

            //Other unimportant MIDI events
            else if ((status >> 4) == 0xA0){ //Polyphonic key pressure
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
            }
            else if ((status >> 4) == 0xB0){ //Control Change
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
            }
            else if ((status >> 4) == 0xC0){ //Program Change
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
            }
            else if ((status >> 4) == 0xD0){ //Channel Pressure
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
            }
            else if ((status >> 4) == 0xE0){ //Pitch Wheel Change
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
            }
            else if (status == 0xF0){ //System Exclusive
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
            }
            else if (status == 0xF2){ //Song Position Pointer
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
            }
            else if (status == 0xF3){ //Song Select
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
            }
            else if (status == 0xF6) //Tune Request
                continue;
            else if (status == 0xF7) //End of Exclusive
                continue;
            else if (status == 0xF8) //Timing Clock
                continue;
            else if (status == 0xFA) //Start
                continue;
            else if (status == 0xFB) //Continue
                continue;
            else if (status == 0xFC) //Stop
                continue;
            else if (status == 0xFE) //Active sensing
                continue;
            else if (status == 0xFF){ //Reset (escape for meta events)
                //Get meta event type
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                char meta_event_type = buf[0];

                //Get meta event length
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                char length = buf[0];

                if (meta_event_type == 0x00){ //Sequence Number
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x01){ //Text Event
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x02){ //Copyright Notice
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Copyright event" << endl;
                }
                else if (meta_event_type == 0x03){ //Sequence/Track Name
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x04){ //Instrument Name
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x05){ //Lyric
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x06){ //Text  Marker
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x07){ //Cue Point
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x20){ //MIDI Channel Prefix
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x2F){ //End of Track
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                else if (meta_event_type == 0x51){ //Set Tempo
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    long tempo = 0;
                    for (i = 0; i < length; i++){
                        tempo = (tempo << 8) | (0x000F & buf[i]);
                    }
                    outfile << total_time << ": Tempo to " << tempo << " usec/quarter note" << endl;
                }
                else if (meta_event_type == 0x54){ //SMPTE Offset
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": SMPTE event" << endl;
                }
                else if (meta_event_type == 0x58){ //Time Signature
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Time Signature event" << endl;
                }
                else if (meta_event_type == 0x59){ //Key Signature
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Key signature event" << endl;
                }
                else if (meta_event_type == 0x7F){ //Sequencer Specific Meta-Event
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                }
                
            }
        }
    }

    //Close files
    infile.close();
    outfile.close();
    clock_t end = clock();
    cout << std::fixed << "Total elapsed time: " <<  double(end - begin) / CLOCKS_PER_SEC << " seconds" << endl;
    return 0;
}

//Helper function to read a number of bytes from MIDI file
void readFromFile(std::fstream& infile, char * bytebuf, int length, long &bytes_left){
    for(int i = 0; i < length; i++)
        infile.read(&(bytebuf[i % BUFFER_SIZE]), 1);
    bytes_left -= length;
}

//Helper function to map proper note and octave
std::string noteFinder(int note_num){
    string this_note = notes[note_num % 12];
    this_note += octaves[(note_num / 12)];
    return this_note;
}