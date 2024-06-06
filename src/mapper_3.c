#include "mapper_3.h"
#include "2C02.h"

/**
 * @brief load initial CPU and PPU address space
 *
 * @param bus
 * @param filename
 */
void cnrom_initialize(MapperInterface *mapper, Bus *bus) {
    // LOAD PROGRAM ROM
    uint16_t prg_rom_start = (mapper->buffer[4] == 2) ? 0x8000 : 0xc000;
    for (int i = 0; i < 0x4000 * mapper->buffer[4]; i++) {
        cpu_write_to_bus(bus, prg_rom_start + i, mapper->buffer[i + 0x10]);
    }

    mapper->allow_cpu_writes = false;

    // LOAD CHR ROM
    uint16_t address_offset = 0x4000 * mapper->buffer[4] + 0x10;
    for (int i = 0; i < 0x2000; i++) {
        ppu_write_to_bus(bus, i, mapper->buffer[i + address_offset]);
    }

    // set mirroring
    set_mirror_mode(bus->ppu, mapper->buffer[6]);
}

/**
 * @brief switch chr bank
 *
 * @param mapper
 * @param bus
 * @param current_address
 * @param chr_bank
 */
void cnrom_switch_chr_banks(struct MapperInterface *mapper, Bus *bus, uint16_t current_address, uint8_t bank_number) {
    if (current_address >= 0x8000) {
        // initialize second 16K window to the last 16K bank (fixed)
        uint16_t address_offset = 0x4000 * mapper->buffer[4] + 0x10;
        uint32_t bank_start = address_offset + (bank_number) * 0x2000;
        for (int i = 0; i < 0x2000; i++) {
            ppu_write_to_bus(bus, i, mapper->buffer[bank_start + i]);
        }
    }
}