#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/2C02.h"
#include "src/6502.h"
#include "src/bus.h"
#include "src/controller.h"
#include "src/mappers.h"
#include "src/window.h"

#define FPS 60

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

    nrom(bus, argv[1]);  // load game's program rom and chr rom

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

    double start = SDL_GetTicks64();
    double end = SDL_GetTicks64();
    double delta = 0;

    while (!quit) {

        end = SDL_GetTicks64();
        delta = end - start;

        while (delta < ((1000.0 / FPS) / 89342)) {
            end = SDL_GetTicks64();
            delta = end - start;
        }
        
        // clock game logic
        clock_bus(bus, window);

        // read input and redner
        pressed_keys = (uint8_t *)SDL_GetKeyboardState(NULL);
        set_controller(controller_1, pressed_keys);
        if (ppu->scanline == 241 && ppu->cycles == 1) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                            quit = true;
                            free(cpu);
                            free(ppu);
                            free(bus);
                            free(controller_1);
                            free(window);
                    }
                }
            }

            SDL_UpdateWindowSurface(window);
            start = SDL_GetTicks64();
        }
        
    }

    return 0;
}