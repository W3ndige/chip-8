all:
	gcc -Wall -Wextra chip8.c -o chip8 -lSDL

clean:
	rm main

tests: chip8
	./chip8 -f './c8games/MERLIN'
