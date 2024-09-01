#include "mapper_4.hpp"

#include "2C02.h"
#include "6502.h"
#include "bus.hpp"

void Mapper_4::initialize() {
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
    this->chr_bank_switch = true;
    if (this->num_chr_banks > 0) {
        uint32_t chr_bank_start = this->prg_bank_size * this->num_prg_banks + 0x10;
        for (int i = 0; i < chr_bank_size; i++) {
            ppu_write_to_bus(bus, i, buffer[i + chr_bank_start]);
        }
    }
    this->chr_bank_switch = false;
}

void Mapper_4::handle_write(uint16_t address, uint8_t value) {
    if (address >= 0x6000 && address <= 0x7fff) {
        this->allow_cpu_writes = true;
        cpu_write_to_bus(bus, address, value);
        this->allow_cpu_writes = false;
    }

    if (address >= 0x8000 && address <= 0x9fff) {
        if (address % 2 == 0)
            this->bank_select.reg = value;
        else {
            this->bank_number = value;
            this->switch_chr_bank();
            this->switch_prg_bank();
        }
    }

    else if (address >= 0xA000 && address <= 0xBFFF) {
        if (address % 2 == 0) {
            this->mirroring = value;
            set_mirror_mode(this->bus->ppu, (this->mirroring & 0x1));
        } else
            this->prg_ram_protect = value;
    }

    else if (address >= 0xC000 && address <= 0xDFFF) {
        if (address % 2 == 0) {
            this->irq_latch = value;
        } else
            this->irq_counter = 0;
    }

    else if (address >= 0xE000 && address <= 0xFFFF) {
        if (address % 2 == 0) {
            this->fire_irq = false;
            this->irq_enable = 0;
        } else {
            this->fire_irq = true;
            this->irq_enable = 1;
        }
    }
}

void Mapper_4::switch_chr_bank() {
    uint16_t address = (0x1000 * this->bank_select.chr_inversion);
    uint16_t bank_size = 0;
    switch (this->bank_select.index) {
        case 0:
            address += 0;
            bank_size = 0x800;
            this->bank_number &= 0xfe;
            break;

        case 1:
            address += 0x800;
            bank_size = 0x800;
            this->bank_number &= 0xfe;
            break;

        case 2:
            address += 0x1000;
            bank_size = 0x400;
            break;

        case 3:
            address += 0x1400;
            bank_size = 0x400;
            break;

        case 4:
            address += 0x1800;
            bank_size = 0x400;
            break;

        case 5:
            address += 0x1c00;
            bank_size = 0x400;
            break;

        default:
            return;
    }

    address %= 0x2000;
    uint32_t bank_start = (0x400 * this->bank_number) + (this->prg_bank_size * this->num_prg_banks) + 0x10;

    this->chr_bank_switch = true;
    for (int i = 0; i < bank_size; i++) {
        ppu_write_to_bus(bus, address + i, this->buffer[bank_start + i]);
    }
    this->chr_bank_switch = false;
}

void Mapper_4::switch_prg_bank() {
    if (this->bank_select.index > 5) {
        this->allow_cpu_writes = true;
        this->bank_number &= 0x3f;
        uint32_t bank_start;
        if (this->bank_select.prg_bank_mode == 0) {
            if (this->bank_select.index == 6) {
                // swap $8000-$9FFF
                bank_start = 0x10 + (this->bank_number * 0x2000);
                for (int i = 0; i < 0x2000; i++) {
                    cpu_write_to_bus(bus, 0x8000 + i, buffer[bank_start + i]);
                }
            }

            else {
                // swap $A000-$BFFF
                bank_start = 0x10 + (this->bank_number * 0x2000);
                for (int i = 0; i < 0x2000; i++) {
                    cpu_write_to_bus(bus, 0xA000 + i, buffer[bank_start + i]);
                }
            }

            // set $C000-$DFFF to second to last bank, $E000-$FFFF to last bank
            bank_start = 0x10 + (this->prg_bank_size * (this->num_prg_banks - 1));
            for (int i = 0; i < 0x4000; i++) {
                cpu_write_to_bus(bus, 0xc000 + i, buffer[bank_start + i]);
            }

        }

        else {
            if (this->bank_select.index == 6) {
                // swap $C000-$DFFF
                bank_start = 0x10 + (this->bank_number * 0x2000);
                for (int i = 0; i < 0x2000; i++) {
                    cpu_write_to_bus(bus, 0xC000 + i, buffer[bank_start + i]);
                }

            }

            else {
                // swap $A000-$BFFF
                bank_start = 0x10 + (this->bank_number * 0x2000);
                for (int i = 0; i < 0x2000; i++) {
                    cpu_write_to_bus(bus, 0xA000 + i, buffer[bank_start + i]);
                }
            }

            // set $8000-$9FFF to second to last bank, $E000-$FFFF to last bank
            bank_start = 0x10 + (this->prg_bank_size) * (this->num_prg_banks - 1);
            for (int i = 0; i < 0x2000; i++) {
                cpu_write_to_bus(bus, 0x8000 + i, buffer[bank_start + i]);
            }

            bank_start = 0x10 + (this->prg_bank_size) * (this->num_prg_banks - 1) + 0x2000;
            for (int i = 0; i < 0x2000; i++) {
                cpu_write_to_bus(bus, 0xe000 + i, buffer[bank_start + i]);
            }
        }
        this->allow_cpu_writes = false;
    }
}

void Mapper_4::check_a12_rising_edge() {
    if (this->bus->a12_state_current == 0) {
        this->a12_low_counter++;
    }

    else {
        if (this->a12_low_counter >= 9) {
            if (this->irq_counter == 0)
                this->irq_counter = this->irq_latch;
            else
                this->irq_counter--;

            if (this->irq_counter == 0 && this->irq_enable)
                irq(this->bus->cpu);
        }

        this->a12_low_counter = 0;
    }
}

void Mapper_4::cleanup() {
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