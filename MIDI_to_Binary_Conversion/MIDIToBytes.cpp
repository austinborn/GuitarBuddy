/* MIDI to Streamable byte format
 *
 * By Austin Born
 * 
 * C++ program to convert notes in MIDI files into a
 * compressed byte map format to send to the ESP32.
 * The byte map will contain basic header information on song name,
 * tempo, and then a sequence of frames for the entire LED array.
 * Each byte in a frame represents one fret of the guitar, so a 
 * single frame will have 10 bytes of data. If there are roughly
 * 32 frames per second, then the byte map for a 5-minute song
 * will be roughly 100 kB. 
 */

/*
 * Curent challenges:
 * - Fix ussues reading certain MIDI files
 * - Fix issues with getting all channel notes
 * - Create new function to convert to binary format instead of .csv
 * - Output proper header data and frame data accordingly
 */

/* About the MIDI Format:
 * Format:
 * <Header Chunk> = <MThd><length><format><ntrks><division>
 * <Track Chunk> = <MTrk><length><MTrk event>+...
 * <MTrk event> = <delta-time><event>
 * <event> = <MIDI event> | <sysex event> | <meta-event>
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

    //If division bit 15 = 1, division[14:8] is negative fps, division[7:0] is ticks per frame
    if(division & 0x8000){
        long fps = -(int)(division & 0x7F00);
        long tpf = division & 0x00FF;
        outfile << "Frames/second - " << fps << endl;
        outfile << "Ticks/frame - " << tpf << endl;
    }
    //Else, division[14:0] is ticks/quarter note
    else{
        long tpq = division & 0x7FFF;
        outfile << "Ticks/quarter note - " << tpq << endl;
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
            if ((status >> 4) == 0x8) { //Note off event

                //Get Note number (and skip velocity)
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                long note_num = buf[0];

                //Record in CSV
                outfile << total_time << ": " << "Off, " << noteFinder(note_num) << ", Channel: " << (status & 0xF) << endl;
            }

            else if ((status >> 4) == 0x9) { //Note on event

                //Get Note number (and skip velocity)
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                long note_num = buf[0];

                //Record in CSV
                outfile << total_time << ": " << "On, " << noteFinder(note_num) << ", Channel: " << (status & 0xF) << endl;
            }

            //Other unimportant MIDI events
            else if ((status >> 4) == 0xA){ //Polyphonic key pressure
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                outfile << total_time << ": Polyphonic key pressure event" << ", Channel: " << (status & 0xF) << endl;
            }
            else if ((status >> 4) == 0xB){ //Control Change
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                outfile << total_time << ": Control change event" << ", Channel: " << (status & 0xF) << endl;
            }
            else if ((status >> 4) == 0xC){ //Program Change
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                outfile << total_time << ": Program change event" << ", Channel: " << (status & 0xF) << endl;
            }
            else if ((status >> 4) == 0xD){ //Channel Pressure
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                outfile << total_time << ": Channel Pressure event" << ", Channel: " << (status & 0xF) << endl;
            }
            else if ((status >> 4) == 0xE){ //Pitch Wheel Change
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                outfile << total_time << ": Pitch wheel event" << ", Channel: " << (status & 0xF) << endl;
            }
            else if (status == 0xF0){ //System Exclusive
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                outfile << total_time << ": System Exclusive event" << endl;
            }
            else if (status == 0xF2){ //Song Position Pointer
                readFromFile(infile, buf, 2, bytes_left);
                trk_bytes_left -= 2;
                outfile << total_time << ": Song position pointer" << endl;
            }
            else if (status == 0xF3){ //Song Select
                readFromFile(infile, buf, 1, bytes_left);
                trk_bytes_left -= 1;
                outfile << total_time << ": Song select event" << endl;
            }
            else if (status == 0xF6){ //Tune Request
                outfile << total_time << ": Tune request" << endl;
                continue;
            }
            else if (status == 0xF7){ //End of Exclusive
                outfile << total_time << ": End of exclusive" << endl;
                continue;
            }
            else if (status == 0xF8){ //Timing Clock
                outfile << total_time << ": Timing clock" << endl;
                continue;
            }
            else if (status == 0xFA){ //Start
                outfile << total_time << ": Start" << endl;
                continue;
            }
            else if (status == 0xFB){ //Continue
                outfile << total_time << ": Continue" << endl;
                continue;
            }
            else if (status == 0xFC){ //Stop
                outfile << total_time << ": Stop" << endl;
                continue;
            }
            else if (status == 0xFE){ //Active sensing
                outfile << total_time << ": Active sensing" << endl;
                continue;
            }
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
                    outfile << total_time << ": Sequence number event" << endl;
                }
                else if (meta_event_type == 0x01){ //Text Event
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Text event" << endl;
                }
                else if (meta_event_type == 0x02){ //Copyright Notice
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Copyright event" << endl;
                }
                else if (meta_event_type == 0x03){ //Sequence/Track Name
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Sequence/Track Name event" << endl;
                }
                else if (meta_event_type == 0x04){ //Instrument Name
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Instrument name event" << endl;
                }
                else if (meta_event_type == 0x05){ //Lyric
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Lyrics event" << endl;
                }
                else if (meta_event_type == 0x06){ //Text  Marker
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Text Marker" << endl;
                }
                else if (meta_event_type == 0x07){ //Cue Point
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": Cue point" << endl;
                }
                else if (meta_event_type == 0x20){ //MIDI Channel Prefix
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": MIDI Channel Prefix" << endl;
                }
                else if (meta_event_type == 0x2F){ //End of Track
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    outfile << total_time << ": End of track " << endl;
                }
                else if (meta_event_type == 0x51){ //Set Tempo
                    readFromFile(infile, buf, length, bytes_left);
                    trk_bytes_left -= length;
                    long tempo = 0;
                    for (i = 0; i < length; i++){
                        tempo = (tempo << 8) | (0x00FF & buf[i]);
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
                    outfile << total_time << ": Sequencer-specific event" << endl;
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