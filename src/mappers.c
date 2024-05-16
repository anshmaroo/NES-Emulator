#include "mappers.h"

/**
 * @brief load an NROM game's program rom and CHR-ROM
 *
 * @param cpu
 * @param filename
 */
void nrom(Bus *bus, char filename[]) {
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
    uint16_t prg_rom_start = (buffer[4] == 2) ? 0x8000 : 0xc000;

    for (int i = 0; i < 0x4000 * buffer[4]; i++) {
        // printf("WRITING $%02x TO $%04x\n", buffer[i + 0x10], prg_rom_start + i);
        cpu_write_to_bus(bus, prg_rom_start + i, buffer[i + 0x10]);
        
    }

    // LOAD CHR ROM
    
    for (int i = 0; i < 0x2000 * buffer[5]; i++) {
        uint16_t address = i + 0x4000 * buffer[4] + 0x10;
        // printf("writing from rom address $%04x to ppu address $%04x\n", address, i);
        ppu_write_to_bus(bus, i, buffer[address]);
    }
}