// IMDIToBytes.cpp

#ifndef ECE445_H 
#define ECE445_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>

void readFromFile(std::fstream& infile, char * bytebuf, int length, long &bytes_left);

std::string noteFinder(int note_num);

#endif 