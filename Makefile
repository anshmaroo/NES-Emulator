make build:
	cc -Wall -O0 -g -arch arm64 -Isrc -I/include/SDL2 -lSDL2 src/*.c main.c -omain 

clean:
	rm main

run:
	./main