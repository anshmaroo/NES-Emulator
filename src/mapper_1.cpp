#include "mapper_1.hpp"

#include "2C02.h"
#include "bus.hpp"

void Mapper_1::initialize() {
    // set prg/chr size/number of banks
    this->prg_bank_size = 0x4000;
    this->chr_bank_size = 0x2000;
    this->num_prg_banks = this->buffer[4];
    this->num_chr_banks = this->buffer[5];

    // load program rom
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0x8000 + i, this->buffer[0x10 + i]);
    }

    // initialize second 16K window to the last 16K bank (fixed)
    uint32_t last_bank_start = ((num_prg_banks - 1) * prg_bank_size) + 0x10;
    for (int i = 0; i < prg_bank_size; i++) {
        cpu_write_to_bus(bus, 0xc000 + i, this->buffer[last_bank_start + i]);
    }

    // LOAD SAVE
    const size_t start_index = 0x1FE0;
    const size_t end_index = 0x3FDF;
    const size_t num_bytes = end_index - start_index + 1;

    char *save_file = (char *)malloc(sizeof(char) * (strlen(game) + 4));
    strcpy(save_file, game);
    strcat(save_file, ".save");

    // Open the file for reading in binary mode
    FILE *file = fopen(save_file, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        // exit(EXIT_FAILURE);
    } 
    
    else {
        if (fread(this->bus->unmapped + start_index, 1, num_bytes, file) != num_bytes) {
            perror("Failed to read from file");
            fclose(file);
            // exit(EXIT_FAILURE);
        }

        fclose(file);
    }

    allow_cpu_writes = false;

    // LOAD CHR ROM
    if (this->num_chr_banks > 0) {
        uint16_t address_offset = prg_bank_size * this->prg_bank_size + 0x10;
        for (int i = 0; i < chr_bank_size; i++) {
            ppu_write_to_bus(bus, i, buffer[i + address_offset]);
        }
    }

    // set mirroring
    set_mirror_mode(bus->ppu, buffer[6] & 0x1);
}

void Mapper_1::handle_write(uint16_t address, uint8_t value) {
    if (address >= 0x6000 && address <= 0x7fff) {
        this->allow_cpu_writes = true;
        cpu_write_to_bus(bus, address, value);
        this->allow_cpu_writes = false;
    }

    else {
        if (value >> 7) {
            this->load = 0;
            this->control.reg = 0;
            this->chr_bank_0 = 0;
            this->chr_bank_1 = 0;
            this->prg_bank.reg = 0;
            this->load_counter = 0;
        }

        else if (this->load_counter < 5) {
            // continue to load register
            this->load |= (value & 0x1) << load_counter;
            this->load_counter++;
        }

        if (this->load_counter == 5) {
            // transfer the 5 bits to the correct register
            if (address <= 0x9fff) {
                // control register
                this->control.reg = this->load;
                set_mirror_mode(this->bus->ppu, this->control.mirror_mode);
                this->prg_bank_size = (this->control.prg_bank_mode <= 1) ? 0x8000 : 0x4000;
                this->chr_bank_size = this->control.chr_bank_mode ? 0x1000 : 0x2000;

            }

            else if (address <= 0xbfff) {
                // chr bank 0 register
                this->chr_bank_0 = this->load;
                this->chr_bank_to_switch = 0;
                switch_chr_bank();
            }

            else if (address <= 0xdfff) {
                // chr bank 1 register
                this->chr_bank_1 = this->load;
                this->chr_bank_to_switch = 1;
                switch_chr_bank();
            }

            else if (address <= 0xffff) {
                // prg bank register
                this->prg_bank.reg = this->load;
                switch_prg_bank();
            }

            this->load = 0;
            this->load_counter = 0;
        }
    }
}

void Mapper_1::switch_chr_bank() {
    if (this->num_chr_banks > 0) {
        if (this->control.chr_bank_mode == 0) {
            // switch entire 8K window
            uint32_t address_offset = 0x4000 * this->num_prg_banks + 0x10;
            uint32_t bank_start = address_offset + (this->chr_bank_0 & 0x1e) * this->chr_bank_size;
            printf("\tBANK: %d, START ADDRESS = $%06x\n", (this->chr_bank_0 & 0x1e), bank_start);
            for (int i = 0; i < this->chr_bank_size; i++) {
                ppu_write_to_bus(bus, i, this->buffer[bank_start + i]);
            }
        }

        else {
            // switch the 4K windows
            uint32_t address_offset = 0x4000 * this->num_prg_banks + 0x10;
            if (this->chr_bank_to_switch == 0) {
                uint32_t bank_0_start = address_offset + this->chr_bank_size * (this->chr_bank_0);
                printf("\tBANK 0: %d, START ADDRESS = $%06x\n", this->chr_bank_0, bank_0_start);
                for (int i = 0; i < this->chr_bank_size; i++) {
                    ppu_write_to_bus(bus, i, this->buffer[bank_0_start + i]);
                }
            }

            else {
                uint32_t bank_1_start = address_offset + this->chr_bank_size * (this->chr_bank_1);
                printf("\tBANK 1: %d, START ADDRESS = $%06x\n", this->chr_bank_1, bank_1_start);
                for (int i = 0; i < this->chr_bank_size; i++) {
                    ppu_write_to_bus(bus, this->chr_bank_size + i, this->buffer[bank_1_start + i]);
                }
            }
        }
    }
}

void Mapper_1::switch_prg_bank() {
    // printf("SWITCHING BANK!\n");
    if (this->control.prg_bank_mode <= 1) {
        // switch entire 32K window
        // printf("\t32K!\n");
        uint32_t bank_start = (this->prg_bank.bank_select >> 1) * 0x8000 + 0x10;
        this->allow_cpu_writes = true;
        for (int i = 0; i < 0x8000; i++) {
            cpu_write_to_bus(bus, 0x8000 + i, this->buffer[bank_start + i]);
        }
        this->allow_cpu_writes = false;
    }

    else if (this->control.prg_bank_mode == 2) {
        // fix first bank, switch last bank
        // printf("\tLAST 16K BANK!\n");
        this->allow_cpu_writes = true;
        for (int i = 0; i < 0x4000; i++) {
            cpu_write_to_bus(bus, 0x8000 + i, this->buffer[0x10 + i]);
        }

        uint32_t bank_start = (this->prg_bank.bank_select) * 0x4000 + 0x10;
        for (int i = 0; i < 0x4000; i++) {
            cpu_write_to_bus(bus, 0xc000 + i, this->buffer[bank_start + i]);
        }

        this->allow_cpu_writes = false;

    }

    else {
        // switch first bank, fix last bank
        // printf("\tFIRST 16K BANK!\n");
        uint32_t bank_start = (this->prg_bank.bank_select) * 0x4000 + 0x10;
        this->allow_cpu_writes = true;
        for (int i = 0; i < 0x4000; i++) {
            cpu_write_to_bus(bus, 0x8000 + i, this->buffer[bank_start + i]);
        }

        bank_start = ((num_prg_banks - 1) * 0x4000) + 0x10;
        for (int i = 0; i < 0x4000; i++) {
            cpu_write_to_bus(bus, 0xc000 + i, this->buffer[bank_start + i]);
        }

        this->allow_cpu_writes = false;
    }
}

void Mapper_1::cleanup() {
    // Define the start and end indexes
    const size_t start_index = 0x1FE0;
    const size_t end_index = 0x3FDF;
    const size_t num_bytes = end_index - start_index + 1;

    // Open the file for writing in binary mode
    char *save_file = (char *)malloc(sizeof(char) * (strlen(game) + 4));
    strcpy(save_file, game);
    strcat(save_file, ".save");
    FILE *file = fopen(save_file, "wb");
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Write the specified range of bytes to the file
    if (fwrite(this->bus->unmapped + start_index, 1, num_bytes, file) != num_bytes) {
        perror("Failed to write to file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    printf("SAVE COMPLETE!\n");

    // Close the file
    fclose(file);
}
