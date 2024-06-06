#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/2C02.h"
#include "src/6502.h"
#include "src/bus.h"
#include "src/controller.h"
#include "src/mapper.h"
#include "src/mapper_0.h"
#include "src/mapper_2.h"
#include "src/mapper_3.h"
#include "src/window.h"

#define SCALE 2

#define FPS 60

int main(int argc, char **argv) {
    // create device objects
    Bus *bus = InitBus();
    MapperInterface *mapper = InitMapper();
    State6502 *cpu = Init6502();
    State2C02 *ppu = Init2C02();
    Controller *controller_1 = InitController();
    Controller *controller_2 = InitController();

    // assign bus to the devices
    cpu->bus = bus;
    ppu->bus = bus;
    controller_1->bus = bus;
    controller_2->bus = bus;

    // assign devices to the bus
    bus->mapper = mapper;
    bus->cpu = cpu;
    bus->ppu = ppu;
    bus->controller_1 = controller_1;
    bus->controller_2 = controller_2;

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
    mapper->buffer = (uint8_t *)malloc(file_size + 1);
    if (!mapper->buffer) {
        fprintf(stderr, "Memory error!");
        fclose(rom);
        return 1;
    }

    // read file contents into buffer
    fread(mapper->buffer, file_size, 1, rom);
    fclose(rom);

    // get the right mapper and set correct functions
    mapper->mapper_number = (mapper->buffer[7] & 0xf0) | (mapper->buffer[6] >> 0x4);
    switch (mapper->mapper_number) {
        case 0:
            // mapper interface for NROM
            mapper->initialize = nrom_initialize;
            mapper->switch_prg_banks = NULL;
            mapper->switch_chr_banks = NULL;
            break;

        case 2:
            // mapper interface for UXROM
            mapper->initialize = uxrom_initialize;
            mapper->switch_prg_banks = uxrom_switch_prg_banks;
            mapper->switch_chr_banks = NULL;
            break;
        
        case 3:
            // mapper interface for CNROM
            mapper->initialize = cnrom_initialize;
            mapper->switch_prg_banks = NULL;
            mapper->switch_chr_banks = cnrom_switch_chr_banks;
            break;

        default:
            exit(0);
            break;
    }

    // initialize addressable space
    mapper->initialize(mapper, bus);

    // set up SDL
    init_SDL();
    SDL_Window *window = create_window(argv[1], SDL_WINDOWPOS_CENTERED, 256 * SCALE, 240 * SCALE);

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

    // initialize timers
    clock_t start = clock();
    clock_t diff = clock() - start;
    while (!quit) {
        // progress logic
        clock_bus(bus, window);

        // read input and redner
        pressed_keys = (uint8_t *)SDL_GetKeyboardState(NULL);
        set_controller(controller_1, pressed_keys);

        if (ppu->scanline == 241 && ppu->cycles == 1) {
            while (((diff * 1000) / CLOCKS_PER_SEC) < (1000.0 / FPS)) {
                diff = clock() - start;
            }

            start = clock();
            diff = clock() - start;

            SDL_UpdateWindowSurface(window);

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                            quit = true;
                            free(cpu);
                            free(ppu);
                            free(mapper);
                            free(bus);
                            free(controller_1);
                            free(window);
                            break;
                    }
                }
            }
        }
    }

    return 0;
}