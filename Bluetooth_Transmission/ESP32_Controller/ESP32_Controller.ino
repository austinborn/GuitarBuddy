
/*
 * ESP32 Bluetooth Controller Program
 * 
 * By Austin Born, Fall 2018
 * 
 * Boilerplate code is in the Public Domain, by Evandro Copercini, 2018
 * 
 * The code creates a bridge betweeon Serial and Classical Bluetooth (SPP),
 * and shows the functionality of SerialBT.
 */

 /*
  * Current challenges:
  * - How to get ESP32 to listen to port from computer
  * - How to save data to large section of memory in ESP32
  * - How to interface with user input buttons on device
  * - How to interface with LED array
  * - How to access data in time with music
  */

#include "BluetoothSerial.h"
#include <stdio.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define BOARDCOUNT 5
#define BUFFERDEPTH 15000

#define CLK 21
#define SDI 22
#define LE 19
#define EN 14
int i = 0;
long frame = 0;
int board = 0;
int unavail = 0;
int line = 0;
//const int buffer_size = 8;
//unsigned char readBuff[buffer_size];
unsigned char hex [4];
int bytes_left = 0;
long file_size = 0;
int file_size_2 = 0;
char frameBuffer[BUFFERDEPTH][BOARDCOUNT];
long frame_count = 0;
bool song_loaded = false;
BluetoothSerial SerialBT;

void updateFrame(){
//    Serial.print("[");
      for(int fret = 0; fret < BOARDCOUNT; fret++){
      sendByte(frameBuffer[frame_count][fret]);
//      Serial.print(frameBuffer[frame_count][fret], BIN);
//      Serial.print(", ");
    }
//    Serial.println("]");
    load();
    frame_count++;
}

void sh() {
  digitalWrite(SDI, HIGH);
  shift();
  digitalWrite(SDI, LOW);

}

void sl() {
  digitalWrite(SDI, LOW);
  shift();

}

void sendByte(byte data) {

  //mask last two bits  since bits 7 and 8 are not used
  char mask = 0xFC; // 0011 1111
  data = mask & data;
  //send data serially over SDI
  //reverse order since first bit in is last bit
  for (int i = 0; i < 8; i++) {
    //shift 1000 0000 to right to change bit
    mask = 0x01 << i;

    //send data unmasked bit
    if (data & mask) {
      sh();
    }
    else {
      sl();
    }
  }
}

void shift() {
  digitalWrite(CLK, HIGH);
  digitalWrite(CLK, LOW);
}

void load() {
  digitalWrite(LE, HIGH);
  
  digitalWrite(LE, LOW);
}

void testBoards(int numOfBoards) {
  for (char i = 0x00; i != 0x40; i++) {
    for (int j = 0; j < numOfBoards; j++) {
      sendByte(i);
    }
    load();
    delay(50);
  }
  for (int k = 0; k < 4; k++) {
    for (char i = 0x01; i != 0x40; i = i << 1) {
      for (int j = 0; j < numOfBoards; j++) {
        sendByte(i);
      }
      load();
      delay(50);
    }
    for (char i = 0x40; i != 0x00; i = i >> 1) {
      for (int j = 0; j < numOfBoards; j++) {
        sendByte(i);
      }
      load();
      delay(50);
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < numOfBoards; j++) {
      sendByte(0xFF);
    }
    load();
    delay(300);
    for (int j = 0; j < numOfBoards; j++) {
      sendByte(0x00);
    }
    load();
    delay(300);
  }
}




void setup() {
  SerialBT.begin("ESP32-GuitarBuddy"); //Bluetooth device name
  Serial.flush();
  Serial.begin(57600);
  pinMode(CLK, OUTPUT);
  pinMode(SDI, OUTPUT);
  pinMode(LE, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH);
  while(1){
    if(SerialBT.available()){
      file_size = (int)SerialBT.read();
      if(file_size == 0){
        delay(100);
        continue;
      }
//      Serial.print(file_size);
      file_size <<= 8;
      delay(200);
      file_size_2 = (int)SerialBT.read();
      file_size += file_size_2;
//      Serial.print(" ");
      Serial.print(file_size);
      break;
      }
  }
  
  bytes_left = file_size;
    while(bytes_left > 0){
      if(SerialBT.available()){
        frameBuffer[frame][board] = SerialBT.read();
        if(board == 4){
          frame++;
          board = 0;
        }
        else
          board++;
        bytes_left -= 1;
      }
    }

  for(int f = 0; f < frame; f ++){
    Serial.print("[");
    for(int fr = 0; fr < BOARDCOUNT; fr++){
      Serial.print(frameBuffer[f][fr], BIN);
      Serial.print(", ");
    }
    Serial.println("]");
  }
}

void loop() {
//  while(1)
//    testBoards(1);
  board = 0;
  frame = 0;
  bytes_left = file_size;
  frame_count = 0;
  while(bytes_left > 0){
    updateFrame();
    bytes_left -= BOARDCOUNT;
    delay(30);
  }
}



//  if(count > 0){
//    Serial.print(line);
//    Serial.write(" ");
//    for(i = 0; i < count; i++){
//      charToHexString(readBuff[i], hex);
//      Serial.write(hex, 4);
//      Serial.write(" ");
//    }
//    Serial.write("\n");
//    
//    if(SerialBT.available()){
//      line += count;
//      unavail = 0;
//    }
//   
//    else if (unavail < 10)
//      unavail++;
//    else
//      line = 0;
//    count = 0;
//  }
  


// void charToHexString(unsigned char chari, unsigned char * hex){
//    int charint = (int)chari;
//    int big = charint/16;
//    int c = 48;
//    hex[0] = '0';
//    hex[1] = 'x';
//    if(big < 10){
//      hex[2] = c+big;
//    }
//    else if(big == 10)
//        hex[2] = 'a';
//    else if(big == 11)
//        hex[2] = 'b';
//    else if(big == 12)
//        hex[2] = 'c';
//    else if(big == 13)
//        hex[2] = 'd';
//    else if(big == 14)
//        hex[2] = 'e';
//    else if(big == 15)
//        hex[2] = 'f';
//
//    int little = charint%16;
//    if(little < 10)
//        hex[3] = c+little;
//    else if(little == 10)
//        hex[3] = 'a';
//    else if(little == 11)
//        hex[3] = 'b';
//    else if(little == 12)
//        hex[3] = 'c';
//    else if(little == 13)
//        hex[3] = 'd';
//    else if(little == 14)
//        hex[3] = 'e';
//    else if(little == 15)
//        hex[3] = 'f';
//    return;
//}
