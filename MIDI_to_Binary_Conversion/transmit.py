import sys
import serial
import os
from array import array
 
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM6'
ser.open()

song = array('B')

file_name = 'BIN_Files/'+str(sys.argv[1])+'.bin'
file_size = os.path.getsize(file_name)

# print(file_name)
# print(file_size)
with open(file_name, 'rb') as file:
    song.fromfile(file, file_size)

ser.write(128)
# ser.write(int(file_size/5))
ser.write(song)
 
# total = 0
 
# while total < len(values):
#     print ord(ser.read(1))
#     total=total+1
 
# ser.close()