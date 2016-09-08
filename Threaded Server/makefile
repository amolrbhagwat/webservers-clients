SHELL:=/bin/bash -O extglob

all: threaded.cpp
	g++ -pthread -o threaded threaded.cpp
   
clean:
	rm !(makefile|threaded.cpp|book.txt)

