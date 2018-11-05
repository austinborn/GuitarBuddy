CC=g++
CFLAGS=-I

make: MIDIToBytes.o
    $(CC) -o MIDIToBytes MIDIToBytes.o

clean:
	rm *.0 MIDIToBytes