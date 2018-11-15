CC=g++
CFLAGS=-I

make: MIDIToBytes.o
	$(CC) -o MIDIToBytes MIDIToBytes.o

cleanwin:
	del /F /Q *.o

cleanlinux:
	rm -f *.o