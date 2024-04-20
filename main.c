#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/2C02.h"
#include "src/6502.h"
#include "src/bus.h"
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
    for (int i = 0; i < 0x6000; i++) {
        cpu_write_to_bus(cpu->bus, 0xc000 + i, buffer[i + 0x10]);
    }

    // LOAD CHR ROM
    for (int i = 0; i < 0x2000; i++) {
        ppu_write_to_bus(cpu->bus, i, buffer[i + 0x4010]);
    }

    // FILE *debug = fopen("debug.txt", "w+");

    // for (int i = 0; i < 0x2000; i++) {
    //     if (i != 0 && i % 16 == 0) {
    //         fprintf(debug, "\n");
    //     }
    //     fprintf(debug, "%02x ", ppu_read_from_bus(cpu->bus, i));
    // }
}

int main(int argc, char **argv) {
    State6502 *cpu = Init6502();
    State2C02 *ppu = Init2C02();
    Bus *bus = InitBus();

    bus->cpu = cpu;
    bus->ppu = ppu;

    ppu->bus = bus;
    cpu->bus = bus;

    loadROM(cpu, argv[1]);  // load game's program rom and chr rom

    SDL_Window *window = create_window(argv[1], 128, 256);

    if (!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        // Handle error appropriately
    }

    render_pattern_tables(ppu, window);

    printf("SHOW WINDOW: \n");
    SDL_ShowWindow(window);

    // Check if the window is shown
    if ((!SDL_GetWindowFlags(window)) & SDL_WINDOW_SHOWN) {
        printf("Failed to show window: %s\n", SDL_GetError());
        // Handle error appropriately
    }

    SDL_Event event;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        quit = true;
                        exit(0);
                }
            }
        }

        // Debug output to check if the surface is updated
        // printf("Updating window surface\n");
        SDL_UpdateWindowSurface(window);

        // Add some delay to prevent busy-waiting
        SDL_Delay(100);
    }

    reset(cpu);  // set pc to reset vector

    while (cpu->pc < 0xffff) {
        emulate6502Op(cpu);
        getchar();
    }
    printf("EXECUTION FINISHED\n");

    return 0;
}