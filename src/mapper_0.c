#include "mapper.h"
#include "bus.h"
#include "2C02.h"


/**
 * @brief load an NROM game's program rom and CHR-ROM
 *
 * @param cpu
 * @param filename
 */
void nrom_initialize(MapperInterface *mapper, Bus *bus) {

    // LOAD PROGRAM ROM
    uint16_t prg_rom_start = (mapper->buffer[4] == 2) ? 0x8000 : 0xc000;
    for (int i = 0; i < 0x4000 * mapper->buffer[4]; i++) {
        cpu_write_to_bus(bus, prg_rom_start + i, mapper->buffer[i + 0x10]);  
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
 * @brief do nothing
 * 
 * @param mapper 
 * @param bus 
 * @param current_address 
 * @param prg_bank 
 */
void nrom_switch_prg_banks(struct MapperInterface *mapper, Bus *bus, uint16_t current_address, uint8_t prg_bank) {

}