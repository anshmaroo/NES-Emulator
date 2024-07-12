#include "mapper_0.hpp"
#include "2C02.h"
#include "bus.hpp"

void Mapper_0::initialize() {
    // set prg/chr size/number of banks
    this->prg_bank_size = 0x4000;
    this->chr_bank_size = 0x2000;
    this->num_prg_banks = this->buffer[4];
    this->num_chr_banks = this->buffer[5];

    // LOAD PROGRAM ROM
    uint16_t prg_rom_start = (num_prg_banks == 2) ? 0x8000 : 0xc000;
    for (int i = 0; i < prg_bank_size * buffer[4]; i++) {
        cpu_write_to_bus(bus, prg_rom_start + i, buffer[i + 0x10]);  
    }

    allow_cpu_writes = false;
    

    // LOAD CHR ROM
    uint16_t address_offset = prg_bank_size * buffer[4] + 0x10;
    for (int i = 0; i < chr_bank_size * buffer[5]; i++) {
        ppu_write_to_bus(bus, i, buffer[i + address_offset]);
    }

    // set mirroring
    set_mirror_mode(bus->ppu, buffer[6] & 0x1);
}
