//#include "BluetoothSerial.h"
//#include "FS.h"
//#include "SD.h"
//#include "SPI.h"


#define CLK 21
#define SDI 22
#define LE 19
#define EN 14

//#define SB1 0
//#define SB2 0
//#define SB3 0
//#define SB4 0
//#define SB5 0
//#define SB6 0
#define BOARDCOUNT 5
#define BUFFERDEPTH 20000

#define VERBOSE false

volatile int updateFrames;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//int frame_count = 0;
//char frameBuffer[BUFFERDEPTH][BOARDCOUNT];



//void IRAM_ATTR frameInterrupt(){
//  portENTER_CRITICAL_ISR(&timerMux);
//  updateFrame();
//  portEXIT_CRITICAL_ISR(&timerMux);
//}

//void writeFile(fs::FS &fs,char* path, char* contents){
//  File file = fs.open(path, FILE_WRITE);
//  if(!file){
//    if(VERBOSE){
//      SerialBT.println("Failed to save file");
//      return;
//    }
//  }
//  file.print(contents);
//  file.close();
//}

//void populateBuffer(fs:FS &fs,char* path){
//  File file = fs.open(path);
//  if(!file){
//    if(VERBOSE) Serial.println('Failed to read file');
//    return
//  }
//  for(int frame = 0; frame < BUFFERDEPTH; frame++){
//    for(int fret = 0; fret < BOARDCOUNT; fret++){
//      if(file.available){
//        frameBuffer[frame][fret] = file.read();
//      }
//    }
//  }
//}

void setFrameRefreshPeriod(int period_ms){
  timerAlarmWrite(timer, period_ms, true);
}


void shift() {
  digitalWrite(CLK, HIGH);
  delay(10);
  digitalWrite(CLK, LOW);
}

void load() {
  digitalWrite(LE, HIGH);
  delay(10);
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

//void updateFrame(){
//  if(frame_count < BUFFERDEPTH){}
//    for(int fret = 0; fret < BOARDCOUNT; fret++){
//      sendByte(frameBuffer[frame_count][fret]);
//    }
//    frame_count++;
//}

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

  //SerialBT.begin("GuitarBuddy");


  Serial.begin(115200);

  //shift register control bits
  pinMode(CLK, OUTPUT);
  pinMode(SDI, OUTPUT);
  pinMode(LE, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH);

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
//  timer = timerBegin(0,80,true);
//  timerAttachInterrupt(timer, &frameInterrupt, true);
//  setFrameRefreshPeriod(1000000);
  //timerAlarmEnable(timer);


}

void loop() {
  testBoards(5);
}
