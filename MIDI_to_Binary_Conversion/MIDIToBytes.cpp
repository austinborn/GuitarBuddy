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
 * - Output proper header data
 * - Deal with channel numbers
 * - MIDI problems: outputting wrong(?) notes at byte 33433 in HotelCalifornia.mid and stuck at end of SugarWereGoinDown.csv
 * - Last note of song
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
#include <cmath>
#include <sstream>
#include <map>
#include "MIDIToBytes.h"

using namespace std;

//Initialize global structures
string notes [12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
string octaves [11] = {"-1", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

map<string, int> note_map = {{"E2", 0}, {"A2", 1}, {"D3", 2}, {"G3", 3}, {"B3", 4}, {"E4", 5},
                            {"F2", 8}, {"A#2", 9}, {"D#3", 10}, {"G#3", 11}, {"C4", 12}, {"F4", 13},
                            {"F#2", 16}, {"B2", 17}, {"E3", 18}, {"A3", 19}, {"C#4", 20}, {"F#4", 21},
                            {"G2", 24}, {"C3", 25}, {"F3", 26}, {"A#3", 27}, {"D4", 28}, {"G4", 29},
                            {"G#2", 32}, {"C#3", 33}, {"F#3", 34}, {"B3", 35}, {"D#4", 36}, {"G#4", 37},
                            {"A4", 45}, {"A#4", 53}, {"B4", 61}, {"C5", 69}, {"C#5", 77}};

//Other constants
const int BUFFER_SIZE = 4;
const int byte_frame_freq = 32;

int main(int argc, char** argv){

    //Initializations
    char buf [BUFFER_SIZE];
    int i = 0;

    //Start clock
    clock_t begin = clock();

    //Open MIDI file
    fstream infile;
    infile.open("MIDI_Files/" + string(argv[1]) + ".mid", fstream::in);

    //Create output file
    fstream outfile;
    outfile.open("CSV_Files/" + string(argv[1]) + ".csv", fstream::in | fstream::out | fstream::trunc);

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
    

    //Initialize Time Variables
    bool tpq_timing = false;
    long fps, tpf, tpq, tempo;
    double time_multiplier = 0;
    short division;

    //Get time division
    readFromFile(infile, buf, 2, bytes_left);
    for (i = 0; i < 2; i++)
        division = (division << 8) + (unsigned char)buf[i];

    //If division bit 15 = 1, division[14:8] is negative fps, division[7:0] is ticks per frame
    if(division & 0x8000){
        fps = -(int)(division & 0x7F00);
        tpf = division & 0x00FF;
        outfile << "Frames/second - " << fps << endl;
        outfile << "Ticks/frame - " << tpf << endl;
    }
    //Else, division[14:0] is ticks/quarter note
    else{
        tpq_timing = true;
        tpq = division & 0x7FFF;
        outfile << "Ticks/quarter note - " << tpq << endl;
    }

    //Initialize track variables
    long trk_length;
    long trk_bytes_left;
    float total_time;
    int round_time;
    int track_num = 0;
    unsigned char prev_status;

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
        round_time = 0;
        unsigned char status;
        bool using_previous;

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
            double delta_float = (double)delta_time;
            delta_float *= time_multiplier;

            total_time += delta_float;
            round_time = round(total_time);

            //Peek status bytes for MIDI event
            status = peekFromFile(infile);

            //Prepare status loop boolean
            using_previous = false;

            do{

                //Parse Status
                if ((status >> 4) == 0x8) { //Note off event
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }

                    //Get Note number (and skip velocity)
                    readFromFile(infile, buf, 2, bytes_left);
                    trk_bytes_left -= 2;
                    unsigned char note_num = buf[0];

                    //Record in CSV
                    outfile << round_time << "," << "Off," << noteFinder(note_num) << ", Channel: " << (status & 0xF) << endl;
                    break;
                }
                else if ((status >> 4) == 0x9) { //Note on event
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                        //Debugging - outfile << int(buf[0]) << "," << int(buf[1]) << endl;
                    }

                    //Get Note number (and skip velocity)
                    readFromFile(infile, buf, 2, bytes_left);
                    trk_bytes_left -= 2;
                    unsigned char note_num = buf[0];
                    //Debugging - outfile << int(buf[0]) << "," << int(buf[1]) << endl;

                    //Record in CSV
                    //Debugging - outfile << "note_num: " << int(note_num) << endl;
                    outfile << round_time << "," << "On," << noteFinder(note_num) << ", Channel: " << (status & 0xF) << endl;
                    break;
                }

                //Other unimportant MIDI events
                else if ((status >> 4) == 0xA){ //Polyphonic key pressure
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 2, bytes_left);
                    trk_bytes_left -= 2;
                    outfile << round_time << ", Polyphonic key pressure event" << ", Channel: " << (status & 0xF) << endl;
                    break;
                }
                else if ((status >> 4) == 0xB){ //Control Change
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 2, bytes_left);
                    trk_bytes_left -= 2;
                    outfile << round_time << ", Control change event" << ", Channel: " << (status & 0xF) << endl;
                    break;
                }
                else if ((status >> 4) == 0xC){ //Program Change
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 1, bytes_left);
                    trk_bytes_left -= 1;
                    outfile << round_time << ", Program change event" << ", Channel: " << (status & 0xF) << endl;
                    break;
                }
                else if ((status >> 4) == 0xD){ //Channel Pressure
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 1, bytes_left);
                    trk_bytes_left -= 1;
                    outfile << round_time << ", Channel Pressure event" << ", Channel: " << (status & 0xF) << endl;
                    break;
                }
                else if ((status >> 4) == 0xE){ //Pitch Wheel Change
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 2, bytes_left);
                    trk_bytes_left -= 2;
                    outfile << round_time << ", Pitch wheel event" << ", Channel: " << (status & 0xF) << endl;
                    outfile << "status:" << (status & 0xFF) << endl;
                    break;
                }
                else if (status == 0xF0){ //System Exclusive
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 1, bytes_left);
                    trk_bytes_left -= 1;
                    outfile << round_time << ", System Exclusive event" << endl;
                    break;
                }
                else if (status == 0xF2){ //Song Position Pointer
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 2, bytes_left);
                    trk_bytes_left -= 2;
                    outfile << round_time << ", Song position pointer" << endl;
                    break;
                }
                else if (status == 0xF3){ //Song Select
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    readFromFile(infile, buf, 1, bytes_left);
                    trk_bytes_left -= 1;
                    outfile << round_time << ", Song select event" << endl;
                    break;
                }
                else if (status == 0xF6){ //Tune Request
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", Tune request" << endl;
                    break;
                }
                else if (status == 0xF7){ //End of Exclusive
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", End of exclusive" << endl;
                    break;
                }
                else if (status == 0xF8){ //Timing Clock
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", Timing clock" << endl;
                    break;
                }
                else if (status == 0xFA){ //Start
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", Start" << endl;
                    break;
                }
                else if (status == 0xFB){ //Continue
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", Continue" << endl;
                    break;
                }
                else if (status == 0xFC){ //Stop
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", Stop" << endl;
                    break;
                }
                else if (status == 0xFE){ //Active sensing
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }
                    outfile << round_time << ", Active sensing" << endl;
                    break;
                }
                else if (status == 0xFF){ //Reset (escape for meta events)
                    if(!using_previous){
                        readFromFile(infile, buf, 1, bytes_left);
                        trk_bytes_left -= 1;
                    }

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
                        outfile << round_time << ", Sequence number event" << endl;
                    }
                    else if (meta_event_type == 0x01){ //Text Event
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Text event" << endl;
                    }
                    else if (meta_event_type == 0x02){ //Copyright Notice
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Copyright event" << endl;
                    }
                    else if (meta_event_type == 0x03){ //Sequence/Track Name
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Sequence/Track Name event" << endl;
                    }
                    else if (meta_event_type == 0x04){ //Instrument Name
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Instrument name event" << endl;
                    }
                    else if (meta_event_type == 0x05){ //Lyric
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Lyrics event" << endl;
                    }
                    else if (meta_event_type == 0x06){ //Text  Marker
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Text Marker" << endl;
                    }
                    else if (meta_event_type == 0x07){ //Cue Point
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Cue point" << endl;
                    }
                    else if (meta_event_type == 0x20){ //MIDI Channel Prefix
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", MIDI Channel Prefix" << endl;
                    }
                    else if (meta_event_type == 0x21){ //MIDI Channel Prefix
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", MIDI Port" << endl;
                    }
                    else if (meta_event_type == 0x2F){ //End of Track
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", End of track " << endl;
                    }
                    else if (meta_event_type == 0x51){ //Set Tempo
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        tempo = 0;
                        for (i = 0; i < length; i++){
                            tempo = (tempo << 8) | (0x00FF & buf[i]);
                        }
                        time_multiplier = tempo*byte_frame_freq*0.000001/tpq;
                        outfile << round_time << ", Tempo to " << tempo << " usec/quarter note" << endl;
                    }
                    else if (meta_event_type == 0x54){ //SMPTE Offset
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", SMPTE event" << endl;
                    }
                    else if (meta_event_type == 0x58){ //Time Signature
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Time Signature event" << endl;
                    }
                    else if (meta_event_type == 0x59){ //Key Signature
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Key signature event" << endl;
                    }
                    else if (meta_event_type == 0x7F){ //Sequencer Specific Meta-Event
                        readFromFile(infile, buf, length, bytes_left);
                        trk_bytes_left -= length;
                        outfile << round_time << ", Sequencer-specific event" << endl;
                    }
                    break;
                }
                else{
                //Use previous status byte as status
                status = prev_status;
                using_previous = true;
                }

            }while(using_previous);

            //Update previous status bytes
            prev_status = status;
        }
    }

    //Convert csv to binary file
    ofstream binfile("BIN_Files/"+string(argv[1]) + ".bin", ios::binary);

    //Prepare byte_map
    char byte_map[10];
    for(int i = 0; i < 10; i++)
        byte_map[i] = 0x00;

    string str_in;
    char * pch;
    int cur_frame, last_frame = 0;
    vector<string> str_vec;
    outfile.clear();
    outfile.seekg(0, ios::beg);
    while(getline(outfile, str_in)){
        char cstr[str_in.size()+1];
        strcpy(cstr, str_in.c_str());
        pch = strtok(cstr,",");
        while(pch != NULL){
            str_vec.push_back(pch);
            pch = strtok(NULL, ",");
        }
        if(str_vec.size() == 4){
            stringstream frame_num(str_vec[0]);
            frame_num >> cur_frame;
            if(cur_frame != last_frame){
                //Print byte_map (cur_frame - last_frame) times
                for(int a = 0; a < (cur_frame - last_frame); a++)
                    for(int i = 0; i < 10; i++)
                        binfile.write(byte_map + i, 1);
                last_frame = cur_frame;
            }
            //Adjust byte_map based on str_vec[2]
            int byte_map_num = note_map[str_vec[2]];
            unsigned char fret_bits = 0x80 >> (byte_map_num % 8);
            int fret = byte_map_num / 8;
            if(str_vec[1] == "On")
                byte_map[fret] |= fret_bits;
            else
                byte_map[fret] &= ~fret_bits;
        }
        while(!str_vec.empty())
            str_vec.pop_back();
    }

    //Close files
    infile.close();
    outfile.close();
    binfile.close();

    //Stop clock
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

//Helper function to peek next 2 bytes from MIDI file (only used for peeking status bytes)
unsigned char peekFromFile(std::fstream& infile){
        return infile.peek();
}

//Helper function to map proper note and octave
std::string noteFinder(int note_num){
    string this_note = notes[note_num % 12];
    this_note += octaves[(note_num / 12)];
    return this_note;
}