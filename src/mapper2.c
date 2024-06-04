#include "bus.h"
#include "2C02.h"
#include "mapper.h"

/**
 * @brief load initial CPU and PPU address space
 *
 * @param bus
 * @param filename
 */
void uxrom_initialize(MapperInterface *mapper, Bus *bus) {
    // LOAD PROGRAM ROM
    // initialize first 16K window to the first 16K bank
    for (int i = 0; i < 0x4000; i++) {
        cpu_write_to_bus(bus, 0x8000 + i, mapper->buffer[0x10 + i]);
    }

    // initialize second 16K window to the last 16K bank (fixed)
    uint32_t last_bank_start = ((mapper->buffer[4] - 1) * 0x4000) + 0x10;
    for (int i = 0; i < 0x4000; i++) {
        cpu_write_to_bus(bus, 0xc000 + i, mapper->buffer[last_bank_start + i]);
    }

    mapper->allow_cpu_writes = false;

    // LOAD CHR ROM
    uint16_t address_offset = 0x4000 * mapper->buffer[4] + 0x10;
    for (int i = 0; i < 0x2000 * mapper->buffer[5]; i++) {
        ppu_write_to_bus(bus, i, mapper->buffer[i + address_offset]);
    }

    // set mirroring
    set_mirror_mode(bus->ppu, mapper->buffer[6]);
}

/**
 * @brief switch prg ROM bank
 *
 * @param bus
 * @param filename
 */
void uxrom_switch_prg_banks(MapperInterface *mapper, Bus *bus, uint16_t current_address, uint8_t bank_number) {
    if (current_address > 0xbfff) {
        // initialize second 16K window to the last 16K bank (fixed)
        uint32_t bank_start = (bank_number) * 0x4000 + 0x10;
        for (int i = 0; i < 0x4000; i++) {
            mapper->allow_cpu_writes = true;
            cpu_write_to_bus(bus, 0x8000 + i, mapper->buffer[bank_start + i]);
            // printf("WRITING $%02x TO $%04X\n", mapper->buffer[bank_start + i], 0x8000 + i);
            mapper->allow_cpu_writes = false;
        }
    }
}