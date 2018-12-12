import sys
import serial
import os
import time
from array import array
 
ser = serial.Serial()
ser.baudrate = 57600
ser.port = 'COM7'#COM10 no work
ser.open()

song = array('B')

file_name = 'BIN_Files/'+str(sys.argv[1])+'.bin'
file_size = os.path.getsize(file_name)

# print(file_name)
# print(file_size)
count = 0
size_1 = file_size & int('0xff00',16)
size_1 >>= 8
size_1 = size_1.to_bytes(1, 'big')
size_2 = file_size & int('0x00ff',16)
size_2 = size_2.to_bytes(1, 'big')
print(size_1)
print(size_2)
ser.write(size_1)
ser.write(size_2)
time.sleep(0.5)
with open(file_name, 'rb') as file:
    for i in range(file_size):
        count += 1
        byte = file.read(1)
        if byte != "":
            ser.write(byte)
            #time.sleep(1)
    # song.fromfile(file, file_size)#file_size
print("sent "+str(count)+" bytes")
 
# total = 0
 
# while total < len(values):
#     print ord(ser.read(1))
#     total=total+1
 
# ser.close()