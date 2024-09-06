#include "mapper_76.hpp"

#include "bus.hpp"
#include "2C02.h"

void Mapper_76::initialize() {
    // set prg/chr size/number of banks
    this->prg_bank_size = 0x4000;
    this->chr_bank_size = 0x2000;
    this->num_prg_banks = this->buffer[4];
    this->num_chr_banks = this->buffer[5];

    printf("Program banks: %d\n", this->num_prg_banks);
    printf("CHR banks: %d\n", this->num_chr_banks);
    

    this->allow_cpu_writes = true;
    // load program rom
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0x8000 + i, this->buffer[0x10 + i]);
    }

    // initialize second 16K window to the last 16K bank (fixed)
    uint32_t last_bank_start = ((num_prg_banks - 1) * prg_bank_size) + 0x10;
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0xc000 + i, this->buffer[last_bank_start + i]);
    }

    this->allow_cpu_writes = false;

    // LOAD CHR ROM
    this->chr_bank_switch = true;
    if (this->num_chr_banks > 0) {
        uint32_t chr_bank_start = this->prg_bank_size * this->num_prg_banks + 0x10;
        for (int i = 0; i < chr_bank_size; i++) {
            ppu_write_to_bus(bus, i, buffer[i + chr_bank_start]);
        }
    }
    this->chr_bank_switch = false;

    // set mirroring
    set_mirror_mode(bus->ppu, buffer[6] & 0x1);
}

void Mapper_76::handle_write(uint16_t address, uint8_t value) {
    address &= 0x8001;
    if (address == 0x8000)
        this->bank_address = value & 0x7;
    else if (address == 0x8001) {
        this->data_port = value;
        if (this->bank_address < 6)
            this->switch_chr_bank();
        else
            this->switch_prg_bank();
    }
}

void Mapper_76::switch_chr_bank() {
    uint32_t bank_start = 0x10 + this->prg_bank_size * this->num_prg_banks + (this->data_port * 0x800);
    uint16_t address;
    switch (this->bank_address) {
        case 2:
            address = 0x0000;
            break;
        
        case 3:
            address = 0x0800;
            break;
        
        case 4:
            address = 0x1000;
            break;
        
        case 5:
            address = 0x1800;
            break;

        default:
            break;
    }

    this->chr_bank_switch = true;
    for (int i = 0; i < 0x800; i++) {
        ppu_write_to_bus(bus, address + i, this->buffer[bank_start + i]);
    }
    this->chr_bank_switch = false;
}

void Mapper_76::switch_prg_bank() {
    uint32_t bank_start = 0x10 + (((this->data_port)) * 0x2000);
    uint16_t address;
    switch (this->bank_address) {
        case 6:
            address = 0x8000;
            break;
        
        case 7:
            address = 0xa000;
            break;
        
        default:
            break;
    }

    this->allow_cpu_writes = true;
    for (int i = 0; i < 0x2000; i++) {
        cpu_write_to_bus(bus, address + i, buffer[bank_start + i]);
    }
    this->allow_cpu_writes = false;
}

void Mapper_76::cleanup() {

}