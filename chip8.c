#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // getopt
#include "SDL/SDL.h"

// TODO(W3ndige) Read the code once again

#define SCREEN_W 640
#define SCREEN_H 320
#define SCREEN_BPP 32

unsigned char chip8_fonts[80] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static int keymap[16] = {
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_4,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_r,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_f,
    SDLK_z,
    SDLK_x,
    SDLK_c,
    SDLK_v

};

struct chip_8 {
    FILE *input_file;
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16]; // CPU registers
    unsigned short I; // Index register
    unsigned short pc; // Program counter
    unsigned char graphics[64 * 32];
    unsigned char delay_timer;
    unsigned char sound_timer;
    unsigned short stack[16];
    unsigned short sp; // Stack pointer
    unsigned char key[16];
} chip8;


void chip8_init(char *file_name) {

    // Program counter initialized at 0x200
    chip8.pc = 0x200;
    chip8.opcode = 0;
    chip8.I = 0;
    chip8.sp = 0;

    // Clear memory, registers, display and stack
    memset(chip8.memory, 0, sizeof(chip8.memory));
    memset(chip8.V, 0, sizeof(chip8.V));
    memset(chip8.graphics, 0, sizeof(chip8.graphics));
    memset(chip8.stack, 0, sizeof(chip8.stack));

    // Read input file
    chip8.input_file = fopen(file_name, "rb");

    if (!chip8.input_file) {
        perror("Error while opening file");
        exit(EXIT_FAILURE);
    }

    // Copy input_file into memory
    fread(chip8.memory + 0x200, 1,  sizeof(chip8.memory) - 0x200, chip8.input_file);

    // Load the fonts into the memory
    for(int i = 0; i < 80; ++i) {
        chip8.memory[i] = chip8_fonts[i];
    }

}

void chip8_timers(){
    if (chip8.delay_timer > 0) {
        chip8.delay_timer--;
    }
    if (chip8.sound_timer > 0) {
        chip8.sound_timer--;
    }
    if (chip8.sound_timer != 0) {
        printf("%c", 7);
    }
}

void chip8_emulate() {
    Uint8 * keys;
    int vx, vy, height;

    // Make it super fast
    for (int times = 0; times < 10; times++) {
        chip8.opcode = chip8.memory[chip8.pc] << 8 | chip8.memory[chip8.pc + 1];
        printf("Executing %04X at %04X \n", chip8.opcode, chip8.pc);
        switch (chip8.opcode & 0xF000) {
            case 0x0000:
                switch (chip8.opcode & 0x000F) {
                    case 0x0000: // Clears the screen
                        memset(chip8.graphics, 0, sizeof(chip8.graphics));
                        chip8.pc += 2;
                        break;
                    case 0x000E: // Returns from subroutine
                        chip8.pc = chip8.stack[--chip8.sp & 0xF] + 2;
                        break;
                }
                break;
            case 0x1000: // Jumps to address NNN
                chip8.pc = chip8.opcode & 0xFFF;
                break;
            case 0x2000: // Calls subroutine at NNN
                chip8.stack[chip8.sp++ & 0xF] = chip8.pc;
                chip8.pc = chip8.opcode & 0x0FFF;
                break;
            case 0x3000: // Skips the next instruction if VX == NN
                if (chip8.V[(chip8.opcode & 0x0F00) >> 8] == (chip8.opcode & 0x00FF)) {
                    chip8.pc += 4;
                }
                else {
                    chip8.pc += 2;
                }
                break;
            case 0x4000: // Skips the next instruction if VX != NN
                if (chip8.V[(chip8.opcode & 0x0F00) >> 8] != (chip8.opcode & 0x00FF)) {
                    chip8.pc += 4;
                }
                else {
                    chip8.pc += 2;
                }
                break;
            case 0x5000: // Skips the next instruction if VX == VY
                if (chip8.V[(chip8.opcode & 0x0F00) >> 8] == chip8.V[(chip8.opcode & 0x00F0) >> 4]) {
                    chip8.pc += 4;
                }
                else {
                    chip8.pc += 2;
                }
                break;
            case 0x6000: // Sets VX to NN.
                chip8.V[(chip8.opcode & 0x0F00) >> 8] = (chip8.opcode & 0x00FF);
                chip8.pc += 2;
                break;
            case 0x7000: // Adds NN to VX
                chip8.V[(chip8.opcode & 0x0F00) >> 8] += (chip8.opcode & 0x00FF);
                chip8.pc += 2;
                break;
            case 0x8000:
                switch (chip8.opcode & 0x000F) {
                    case 0x0000: // Sets VX to the value of VY
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x0F00) >> 4];
                        chip8.pc += 2;
                        break;
                    case 0x0001: // Sets VX to VX or VY
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x0F00) >> 8] | chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.pc += 2;
                        break;
                    case 0x0002: // 8XY2: Sets VX to VX and VY
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x0F00) >> 8] & chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.pc += 2;
                        break;
                    case 0x0003: // Sets VX to VX XOR VY
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x0F00) >> 8] ^ chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.pc += 2;
                        break;
                    case 0x0004: // Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't
                        if (((int)chip8.V[(chip8.opcode & 0x0F00) >> 8 ] + (int)chip8.V[(chip8.opcode & 0x00F0) >> 4]) < 256) {
                            chip8.V[0xF] &= 0;
                        }
                        else {
                            chip8.V[0xF] = 1;
                        }
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] += chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.pc += 2;
                        break;
                    case 0x0005: // VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                        if (((int)chip8.V[(chip8.opcode & 0x0F00) >> 8 ] - (int)chip8.V[(chip8.opcode & 0x00F0) >> 4]) >= 0) {
                            chip8.V[0xF] = 1;
                        }
                        else {
                            chip8.V[0xF] &= 0;
                        }
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] += chip8.V[(chip8.opcode & 0x00F0) >> 4];
                        chip8.pc += 2;
                        break;
                    case 0x0006: // Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift
                        chip8.V[0xF] = chip8.V[(chip8.opcode & 0x0F00) >> 8] & 7;
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x0F00) >> 8] >> 1;
                        chip8.pc += 2;
                        break;
                    case 0x0007: // Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
                        if (((int)chip8.V[(chip8.opcode & 0x0F00) >> 8] - (int)chip8.V[(chip8.opcode & 0x00F0) >> 4]) > 0) {
                            chip8.V[0xF] = 1;
                        }
                        else {
                            chip8.V[0xF] &= 0;
                        }

                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x00F0) >> 4] - chip8.V[(chip8.opcode & 0x0F00) >> 8];
                        chip8.pc += 2;
                        break;
                    case 0x000E: // Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift
                        chip8.V[0xF] = chip8.V[(chip8.opcode & 0x0F00) >> 8] >> 7;
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.V[(chip8.opcode & 0x0F00) >> 8] << 1;
                        chip8.pc += 2;
                        break;
                }
                break;
            case 0x9000: // Skips the next instruction if VX doesn't equal VY
                if(chip8.V[(chip8.opcode & 0x0F00) >> 8] != chip8.V[(chip8.opcode & 0x00F0) >> 4]) {
                    chip8.pc += 4;
                }
                else {
                    chip8.pc += 2;
                }
                break;
            case 0xA000: // Sets I to the address NNN
                chip8.I = chip8.opcode & 0x0FFF;
                chip8.pc += 2;
                break;
            case 0xB000: // Jumps to the address NNN plus V0
                chip8.pc = (chip8.opcode & 0x0FFF) + chip8.V[0];
                break;
            case 0xC000: // Sets VX to a random number and NN
                chip8.V[(chip8.opcode & 0x0F00) >> 8] = rand() & (chip8.opcode & 0x00FF);
                chip8.pc += 2;
                break;
            case 0xD000: // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels
                vx = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                vy = chip8.V[(chip8.opcode & 0x00F0) >> 4];
                height = chip8.opcode & 0x000F;
                chip8.V[0xF] &= 0;

                for (int y = 0; y < height; y++) {
                    int pixel = chip8.memory[chip8.I + y];
                    for (int x = 0; x < 8; x++) {
                        if(pixel & (0x80 >> x)) {
                            if(chip8.graphics[x+vx+(y+vy)*64]) {
                                chip8.V[0xF] = 1;
                            }
                            chip8.graphics[x+vx+(y+vy)*64] ^= 1;
                        }
                    }
                }

                chip8.pc += 2;
                break;
            case 0xE000: // Skips the next instruction if the key stored in VX is pressed
                switch (chip8.opcode & 0x00F) {
                    case 0x000E:
                        keys = SDL_GetKeyState(NULL);
                        if(keys[keymap[chip8.V[(chip8.opcode & 0x0F00) >> 8]]]) {
                            chip8.pc += 4;
                        }
                        else {
                            chip8.pc += 2;
                        }
                        break;
                    case 0x0001: // Skips the next instruction if the key stored in VX isn't pressed
                        keys = SDL_GetKeyState(NULL);
                        if(!keys[keymap[chip8.V[(chip8.opcode & 0x0F00) >> 8]]]) {
                            chip8.pc += 4;
                        }
                        else {
                            chip8.pc += 2;
                        }
                        break;
                }
                break;
            case 0xF000:
                switch (chip8.opcode & 0x00FF) {
                    case 0x0007: // Sets VX to the value of the delay timer
                        chip8.V[(chip8.opcode & 0x0F00) >> 8] = chip8.delay_timer;
                        chip8.pc += 2;
                        break;
                    case 0x000A: // A key press is awaited, and then stored in VX
                        keys = SDL_GetKeyState(NULL);
                        for (int i = 0; i < 0x10; i++) {
                            if (keys[keymap[i]]) {
                                chip8.V[(chip8.opcode & 0x0F00) >> 8] = i;
                                chip8.pc += 2;
                            }
                        }
                        break;
                    case 0x0015: // Sets the delay timer to VX
                        chip8.delay_timer = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                        chip8.pc += 2;
                        break;
                    case 0x0018: // Sets the sound timer to VX
                        chip8.sound_timer = chip8.V[(chip8.opcode & 0x0F00) >> 8];
                        chip8.pc += 2;
                        break;
                    case 0x001E: // Adds VX to I
                        chip8.I += chip8.V[(chip8.opcode & 0x0F00) >> 8];
                        chip8.pc += 2;
                        break;
                    case 0x0029: // Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
                        chip8.I = chip8.V[(chip8.opcode & 0x0F00) >> 8] * 5;
                        chip8.pc += 2;
                        break;
                    case 0x0033: // Stores the Binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, and the least significant digit at I plus 2
                        chip8.memory[chip8.I] = chip8.V[(chip8.opcode & 0x0F00) >> 8] / 100;
                        chip8.memory[chip8.I+1] = (chip8.V[(chip8.opcode & 0x0F00) >> 8] / 10) % 10;
                        chip8.memory[chip8.I+2] = chip8.V[(chip8.opcode & 0x0F00) >> 8] % 10;
                        chip8.pc += 2;
                        break;

                    case 0x0055: // Stores V0 to VX in memory starting at address I
                        for (int i = 0; i <= ((chip8.opcode & 0x0F00) >> 8); i++) {
                            chip8.memory[chip8.I+i] = chip8.V[i];
                        }
                        chip8.pc += 2;
                        break;

                    case 0x0065: // Fills V0 to VX with values from memory starting at address I
                        for (int i = 0; i <= ((chip8.opcode & 0x0F00) >> 8); i++) {
                            chip8.V[i] = chip8.memory[chip8.I + i];
                        }
                        chip8.pc += 2;
                        break;

                }
                break;
        }
        chip8_timers();
    }

}

void chip8_draw() {
    int i, j;
    SDL_Surface * surface = SDL_GetVideoSurface();
    SDL_LockSurface(surface);
    Uint32 * screen = (Uint32 *)surface->pixels;
    memset(screen,0,surface->w*surface->h*sizeof(Uint32));
    for (i = 0; i < SCREEN_H; i++)
        for (j = 0; j < SCREEN_W; j++){
            screen[j+i*surface->w] = chip8.graphics[(j/10)+(i/10)*64] ? 0xFFFFFFFF : 0;
        }

    SDL_UnlockSurface(surface);
    SDL_Flip(surface);
    SDL_Delay(15);
}

void chip8_prec(char * file_name, SDL_Event * event){
    Uint8 * keys = SDL_GetKeyState(NULL);
    if(keys[SDLK_ESCAPE])
        exit(1);
    if(keys[SDLK_r])
        chip8_init(file_name);
    if(keys[SDLK_p]){
        while(1){
            if(SDL_PollEvent(event)){
                keys = SDL_GetKeyState(NULL);
                if(keys[SDLK_ESCAPE])
                    exit(1);
            }
        }
    }
}


void chip8_run(char *file_name) {

    // Prepare SDL
    SDL_Event event;
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_SetVideoMode(SCREEN_W, SCREEN_H, SCREEN_BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);

    chip8_init(file_name);
    int quit = 0;

    while (!quit) {
        // Handle SDL_QUIT in loop
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }


        chip8_emulate();
        chip8_draw();
        chip8_prec(file_name, &event);
        }
}


int main(int argc, char *argv[]) {


    char *file_name = "";

    // Parameter parsing
    int opt;
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
        case 'f':
            file_name = optarg;
            break;
        }
    }

    chip8_run(file_name);

    return 0;
}
