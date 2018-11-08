'''
Bluetooth Serial Read Program

By Austin Born, Fall 2018

Reads data from specific Bluetooth port to communicate with ESP32.
While there is data available, receive data. This program will
be used largely to send data to the ESP32 and confirm that the data
has been properly saved on the ESP32.
'''

'''
Current challenges:
- How to find the binary file to transmit
- How to send data over a socket
'''

import time
import serial

port = 'COM6'
ser = serial.Serial(port, 115200, timeout=0)

while True:
    data = ser.read(32)
    if len(data) > 0:
        print('Got: ' + str(data)[2:-1]  )
    time.sleep(128/115200)
    # print('not blocked')

ser.close()