#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/2C02.h"
#include "src/6502.h"
#include "src/bus.h"
#include "src/controller.h"
#include "src/window.h"

/**
 * @brief load an NROM game's program rom and CHR-ROM
 *
 * @param cpu
 * @param filename
 */
void loadROM(State6502 *cpu, char filename[]) {
    FILE *f = fopen(filename, "rb");  // open ROM file

    if (f == NULL) {
        printf("error: Could not open %s\n", filename);
        exit(1);
    }

    // get file size, read it into a buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    uint8_t *buffer = malloc(fsize);
    fread(buffer, fsize, 1, f);
    fclose(f);

    // LOAD PROGRAM ROM
    for (int i = 0; i < 0x4000; i++) {
        cpu_write_to_bus(cpu->bus, 0xc000 + i, buffer[i + 0x10]);
    }

    // LOAD CHR ROM
    for (int i = 0; i < 0x2000; i++) {
        ppu_write_to_bus(cpu->bus, i, buffer[i + 0x4010]);
    }
}

int main(int argc, char **argv) {
    State6502 *cpu = Init6502();
    State2C02 *ppu = Init2C02();
    Bus *bus = InitBus();
    Controller *controller_1 = InitController();

    cpu->bus = bus;
    ppu->bus = bus;
    controller_1->bus = bus;

    bus->cpu = cpu;
    bus->ppu = ppu;
    bus->controller_1 = controller_1;

    loadROM(cpu, argv[1]);  // load game's program rom and chr rom

    SDL_Window *window = create_window(argv[1], 512, 480);

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

    SDL_Event event;
    bool quit = false;

    reset(cpu);

    uint8_t *pressed_keys = (uint8_t *)SDL_GetKeyboardState(NULL);

    unsigned int a = SDL_GetTicks();
    unsigned int b = SDL_GetTicks();
    double delta = 0;

    while (!quit) {
        pressed_keys = (uint8_t *)SDL_GetKeyboardState(NULL);
        set_controller(controller_1, pressed_keys);
        clock_bus(bus, window);

        if (ppu->scanline == 241 && ppu->cycles == 1) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                            quit = true;
                            free(cpu);
                            free(ppu);
                            free(bus);
                            free(window);
                    }
                }
            }

            // printf("fps: %f\n", (1000 / delta));
            b = a;
            SDL_UpdateWindowSurface(window);

            a = SDL_GetTicks();
            delta = a - b;
        }
    }

    return 0;
}