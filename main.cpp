#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/2C02.h"
#include "src/6502.h"
#include "src/bus.hpp"
#include "src/controller.h"
#include "src/mapper.hpp"
#include "src/mapper_0.hpp"
#include "src/mapper_1.hpp"
#include "src/mapper_2.hpp"
#include "src/mapper_3.hpp"
#include "src/window.h"


int main(int argc, char **argv) {

    // get basic game info
    char *game = (char *) malloc(sizeof(char) * 200);
    strcpy(game, argv[1]);
    game[strlen(argv[1]) - 4] = '\0';
    

    // create device objects
    Bus *bus = InitBus();
    Mapper *mapper;
    State6502 *cpu = Init6502();
    State2C02 *ppu = Init2C02();
    Controller *controller_1 = InitController();
    Controller *controller_2 = InitController();

    // assign bus to the devices
    cpu->bus = bus;
    ppu->bus = bus;
    controller_1->bus = bus;
    // controller_2->bus = bus;

    // assign devices to the bus
    bus->cpu = cpu;
    bus->ppu = ppu;
    bus->controller_1 = controller_1;
    // bus->controller_2 = controller_2;

    // load the rom into a buffer
    FILE *rom = fopen(argv[1], "rb");

    if (!rom) {
        fprintf(stderr, "Unable to open rom %s.\n", argv[1]);
        return 1;
    }

    // get file length
    fseek(rom, 0, SEEK_END);
    int file_size = ftell(rom);
    fseek(rom, 0, SEEK_SET);

    // create buffer and allocate memory
    uint8_t *buffer = (uint8_t *)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory error!");
        fclose(rom);
        return 1;
    }

    // read file contents into buffer
    fread(buffer, file_size, 1, rom);
    fclose(rom);

    // get the right mapper and set correct functions
    uint16_t mapper_number = (buffer[7] & 0xf0) | (buffer[6] >> 0x4);
    switch (mapper_number) {
        case 0:
            // mapper interface for NROM
            mapper = new Mapper_0(game, mapper_number, buffer, bus);
            break;

        case 1:
            // mapper interface for MMC1
            mapper = new Mapper_1(game, mapper_number, buffer, bus);
            break;

        case 2:
            // mapper interface for UNROM
            mapper = new Mapper_2(game, mapper_number, buffer, bus);
            break;

        case 3:
            // mapper interface for CNROM
            mapper = new Mapper_3(game, mapper_number, buffer, bus);
            break;

        default:
            exit(0);
            break;
    }

    // initialize addressable space
    bus->mapper = mapper;
    mapper->initialize();

    // set up SDL and window
    int scale = 2;
    if (argc == 3) {
        scale = atoi(argv[2]);
    }
    init_SDL();
    SDL_Window *window = create_window(game, SDL_WINDOWPOS_CENTERED, 256 * scale, 240 * scale);

    SDL_Event event;
    bool quit = false;

    if (!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        // Handle error appropriately
    }

    SDL_ShowWindow(window);

    // Check if the window is shown
    if ((!SDL_GetWindowFlags(window)) & SDL_WINDOW_SHOWN) {
        printf("Failed to show window: %s\n", SDL_GetError());
        // Handle error appropriately
    }

    // initialize input
    uint8_t *pressed_keys = (uint8_t *)SDL_GetKeyboardState(NULL);

    // reset cpu
    reset(cpu);

    // initialize timers/fps
    clock_t start = clock();
    clock_t diff = clock() - start;
    
    int fps = 60;
    if (argc == 4) {
        fps = atoi(argv[3]);
    }

    bool paused = false;

    while (!quit) {
        // progress logic
        if(!paused)
            clock_bus(bus, window);

        // read input
        pressed_keys = (uint8_t *)SDL_GetKeyboardState(NULL);
        set_controller(controller_1, pressed_keys);

        // render only after vblank
        if ((!paused && ppu->scanline == 241 && ppu->cycles == 1) || paused) {
            while (((diff * 1000) / CLOCKS_PER_SEC) < (1000.0 / fps)) {
                diff = clock() - start;
            }

            start = clock();
            diff = clock() - start;

            SDL_UpdateWindowSurface(window);

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            quit = true;
                            mapper->cleanup();
                            free(cpu);
                            free(ppu);
                            free(mapper);
                            free(bus);
                            free(controller_1);
                            free(window);
                            break;
                        
                        case SDLK_p:
                            paused = !paused;
                            break;

                    }
                }
            }
        }
    }

    return 0;
}