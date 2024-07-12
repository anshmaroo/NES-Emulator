#include "mapper_2.hpp"
#include "bus.hpp"
#include "2C02.h"

void Mapper_2 ::initialize() {
    // set prg/chr size/number of banks
    this->prg_bank_size = 0x4000;
    this->chr_bank_size = 0x2000;
    this->num_prg_banks = this->buffer[4];
    this->num_chr_banks = this->buffer[5];

    // LOAD PROGRAM ROM
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0x8000 + i, this->buffer[0x10 + i]);
    }

    // initialize second 16K window to the last 16K bank (fixed)
    uint32_t last_bank_start = ((num_prg_banks - 1) * prg_bank_size) + 0x10;
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0xc000 + i, this->buffer[last_bank_start + i]);
    }
    allow_cpu_writes = false;

    // LOAD CHR ROM
    uint16_t address_offset = prg_bank_size * buffer[4] + 0x10;
    for (int i = 0; i < chr_bank_size * buffer[5]; i++) {
        ppu_write_to_bus(bus, i, buffer[i + address_offset]);
    }

    // set mirroring
    set_mirror_mode(bus->ppu, buffer[6]);
}

void Mapper_2::handle_write(uint16_t address, uint8_t value) {
    // printf("SWITCHING PRG BANK!\n");
    if (address > 0xbfff) {
        switch_prg_bank(value);
    }
}

void Mapper_2::switch_prg_bank(uint8_t prg_bank_number) {
    uint32_t bank_start = (prg_bank_number) * prg_bank_size + 0x10;
    this->allow_cpu_writes = true;
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0x8000 + i, this->buffer[bank_start + i]);
    }
    this->allow_cpu_writes = false;
}
