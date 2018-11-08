
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

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define TXPIN 25
#define RXPIN 35

unsigned char mod = 0x12;
int i = 0;
int count = 0;
unsigned char readBuff;
BluetoothSerial SerialBT;

void setup() {
  SerialBT.begin("ESP32-GuitarBuddy"); //Bluetooth device name
  initializeRawDataMode();
  Serial.flush();
  Serial.begin(57600);
}
id loop() {
  SerialBT.write(Serial.read());
}

//Example code from another project
void initializeRawDataMode(){
  for (int i = 0; i < 20; i++){
    Serial.begin(9600);
	  Serial.write(mod);
  }
}
