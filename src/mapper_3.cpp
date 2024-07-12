#include "mapper_3.hpp"
#include "bus.hpp"
#include "2C02.h"

void Mapper_3::initialize() {
    // set prg/chr size/number of banks
    this->prg_bank_size = 0x4000;
    this->chr_bank_size = 0x2000;
    this->num_prg_banks = this->buffer[4];
    this->num_chr_banks = this->buffer[5];

    // LOAD PROGRAM ROM
    uint16_t prg_rom_start = (this->num_prg_banks == 2) ? 0x8000 : 0xc000;
    for (int i = 0; i < this->prg_bank_size * this->num_prg_banks; i++) {
        cpu_write_to_bus(bus, prg_rom_start + i, this->buffer[i + 0x10]);
    }

    this->allow_cpu_writes = false;

    // LOAD CHR ROM
    uint16_t address_offset = 0x4000 * this->num_prg_banks + 0x10;
    for (int i = 0; i < 0x2000; i++) {
        ppu_write_to_bus(bus, i, this->buffer[i + address_offset]);
    }

    // set mirroring
    set_mirror_mode(bus->ppu, this->buffer[6]);
}

void Mapper_3::handle_write(uint16_t address, uint8_t value) {
    // printf("SWITCHING PRG BANK!\n");
    if (address > 0xbfff) {
        switch_chr_bank(value & 0x3);
    }
}

void Mapper_3::switch_chr_bank(uint8_t chr_bank_number) {
    // switch character rom bank
    uint16_t address_offset = prg_bank_size * this->buffer[4] + 0x10;
    uint32_t bank_start = address_offset + (chr_bank_number)*chr_bank_size;
    for (int i = 0; i < chr_bank_size; i++) {
        ppu_write_to_bus(bus, i, this->buffer[bank_start + i]);
    }
}

