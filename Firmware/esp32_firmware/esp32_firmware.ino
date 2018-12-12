#include "BluetoothSerial.h"
//#include "FS.h"
//#include "SD.h"
//#include "SPI.h"


#define CLK 21
#define SDI 22
#define LE 19
#define BUTTON 14
#define BLED 16
#define RLED 17

//#define SB1 0
//#define SB2 0
//#define SB3 0
//#define SB4 0
//#define SB5 0
//#define SB6 0
#define BOARDCOUNT 5
#define BUFFERDEPTH 781

#define VERBOSE false

volatile int updateFrames;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

int frame_count = 0;
char songData[BUFFERDEPTH][BOARDCOUNT];

BluetoothSerial SerialBT;

//void IRAM_ATTR frameInterrupt(){
//  portENTER_CRITICAL_ISR(&timerMux);
//  updateFrame();
//  portEXIT_CRITICAL_ISR(&timerMux);
//}

void setFrameRefreshPeriod(int period_ms){
  timerAlarmWrite(timer, period_ms, true);
}


void shift() {
  digitalWrite(CLK, HIGH);
  delayMicroseconds(100);
  digitalWrite(CLK, LOW);
}

void load() {
  digitalWrite(LE, HIGH);
  delayMicroseconds(100);
  digitalWrite(LE, LOW);
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
  char mask = 0x3F; // 0011 1111
  data = mask & data;
  //send data serially over SDI
  //reverse order since first bit in is last bit
  for (int i = 0; i < 8; i++) {
    //shift 1000 0000 to right to change bit
    mask = 0x80 >> i;

    //send data unmasked bit
    if (data & mask) {
      sh();
    }
    else {
      sl();
    }
  }
}

void updateFrame(){
  if(frame_count < BUFFERDEPTH){}
    for(int fret = 0; fret < BOARDCOUNT; fret++){
      sendByte(songData[frame_count][fret]);
    }
    frame_count++;
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

  SerialBT.begin("GuitarBuddy");


  Serial.begin(115200);
  //SerialBT.begin("GuitarBuddy");

  //shift register control bits
  pinMode(CLK, OUTPUT);
  pinMode(SDI, OUTPUT);
  pinMode(LE, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(BLED, OUTPUT);
  pinMode(RLED, OUTPUT);


  //digitalWrite(BLED, HIGH);
  //digitalWrite(RLED, HIGH);
  //string data bus
  //  pinMode(SB1, INPUT);
  //  pinMode(SB2, INPUT);
  //  pinMode(SB3, INPUT);
  //  pinMode(SB4, INPUT);
  //  pinMode(SB5, INPUT);
  //  pinMode(SB6, INPUT);
  //
  //  //enable sensing module
  //  pinMode(EN, OUTPUT);
  //
  //  digitalWrite(TRIBUFEN, LOW);


  //initialize frame timer
  //timer = timerBegin(0,80,true);
//  timerAttachInterrupt(timer, &frameInterrupt, true);
//  setFrameRefreshPeriod(1000000);
//  timerAlarmEnable(timer);
  songData[0][0] = 0x00;
  songData[0][0] = 0x01;
  songData[0][0] = 0x02;
  songData[0][0] = 0x03;
  songData[0][0] = 0x04;
  for(int frame = 1; frame < BUFFERDEPTH; frame++){
    songData[frame][0] = (songData[frame-1][BOARDCOUNT-1]+1)%0x40;
    for(int fret = 1 ; fret < BOARDCOUNT; fret++){
      songData[frame][fret] = (songData[frame][fret-1] + 1) % 0x40;
    }
  }

}

void loop() {
  testBoards(5);
}
//  if(digitalRead(BUTTON)){
//    testBoards(5);
//  }
//  else{
//    digitalWrite(BLED, HIGH);
//    digitalWrite(RLED, HIGH);
//  }
//  testBoards(5);

//    if(SerialBT.available()){
//      Serial.println(SerialBT.read(), HEX);
//    if(SerialBT.available()){
//      char magicFlagBuffer[1] = {0x00};
//      Serial.println(magicFlagBuffer[0]);
//      if(SerialBT.readBytes(magicFlagBuffer, 1) == 0xFF){
//        int numOfFrames = SerialBT.read();
//        for(int frame_num = 0; frame_num < numOfFrames && frame_num < BUFFERDEPTH; frame_num++){
//           char frame[BOARDCOUNT];
//           SerialBT.readBytes(frame, BOARDCOUNT);
//           for(int fret = 0; fret < BOARDCOUNT; fret++){
//            songData[numOfFrames][fret] = frame[fret];
//          }
//        }
//      }
  //   testBoards(5);
  //   if(SerialBT.available()){
      
  //   }
  // }
   
