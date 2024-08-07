#include "bus.hpp"

#include "2C02.h"
#include "6502.h"
#include "controller.h"
#include "mapper.hpp"

void cpu_write_to_bus(Bus *bus, uint16_t address, uint8_t value) {
    if (address <= 0x1fff) {
        // CPU RAM
        address &= 0x07ff;
        bus->cpu_ram[address] = value;
    }

    else if (address <= 0x3fff) {
        // PPU REGISTERS
        address &= 0x0007;
        // printf("CPU WRITING %02x TO REGISTER 20%02x\n", value, address);

        write_to_ppu_register(bus->ppu, address, value);
        // getchar();
    }

    else if (address <= 0x4017) {
        // APU/IO REGISTERS
        if (address == 0x4014)
            write_to_ppu_register(bus->ppu, address, value);
        else if (address == 0x4016) {
            // CONTROLLER
            if ((value & 0x1) == 1) {
                bus->poll_input1 = -1;
            }

            else if ((value & 0x1) == 0) {
                bus->poll_input1 = 0;
            }
        }

        else {
            address &= 0x0014;
            bus->apu_io_registers[address] = value;
        }
    }

    else if (address >= 0x4020 && address <= 0xffff) {
        if (bus->mapper->allow_cpu_writes) {
            address -= 0x4020;
            bus->unmapped[address] = value;
        }

        else {
            bus->mapper->handle_write(address, value);
        }
    }
}

uint8_t cpu_read_from_bus(Bus *bus, uint16_t address) {
    uint8_t value = 0;
    if (address <= 0x1fff) {
        address &= 0x07ff;
        value = bus->cpu_ram[address];
    }

    else if (address <= 0x3fff) {
        address &= 0x0007;
        // printf("CPU READING %02x FROM REGISTER 20%02x\n", value, address);
        value = read_from_ppu_register(bus->ppu, address);

    }

    else if (address <= 0x4017) {
        if (address == 0x4014) {
            value = read_from_ppu_register(bus->ppu, address);
        }

        else if (address == 0x4016) {
            bool bit = 0;
            if (bus->poll_input1 >= 0) {
                bit = read_from_controller(bus->controller_1, bus->poll_input1++);
                if (bus->poll_input1 > 7)
                    bus->poll_input1 = -1;
                value = 0x40 | bit;
                // printf("INPUT VALUE = %02x\n", value);
            }

            else if (bus->poll_input1 == -1) {
                value = 0x40 | read_from_controller(bus->controller_1, 0);
                // printf("STROBE READ = %02x\n", value);
            }

        }

        else if (address = 0x4017) {
            value = 0;
        }

    }

    else if (address >= 0x4020 && address <= 0xffff) {
        address -= 0x4020;
        value = bus->unmapped[address];
    }

    return value;
}

void ppu_write_to_bus(Bus *bus, uint16_t address, uint8_t value) {
    if (address <= 0x0fff) {
        bus->pattern_table_0[address] = value;
    }

    else if (address <= 0x1fff) {
        address &= 0x0fff;
        bus->pattern_table_1[address] = value;
    }

    else if (address <= 0x2fff) {
        if (bus->ppu->mirror_mode == SINGLE_SCREEN_LOWER) {
            address &= 0x03ff;
            bus->name_table_0[address] = value;
        }

        else if (bus->ppu->mirror_mode == SINGLE_SCREEN_UPPER) {
            address &= 0x03ff;
            bus->name_table_1[address] = value;
        }

        else {
            if (address <= 0x23ff) {
                address &= 0x03ff;
                bus->name_table_0[address] = value;
            }

            else if (address <= 0x27ff) {
                address &= 0x03ff;
                if (bus->ppu->mirror_mode == HORIZONTAL)
                    bus->name_table_0[address] = value;
                else
                    bus->name_table_1[address] = value;
            }

            else if (address <= 0x2bff) {
                address &= 0x03ff;
                if (bus->ppu->mirror_mode == HORIZONTAL)
                    bus->name_table_1[address] = value;
                else
                    bus->name_table_0[address] = value;
            }

            else if (address <= 0x2fff) {
                address &= 0x03ff;
                bus->name_table_1[address] = value;
            }
        }
    }

    else if (address >= 0x3f00 && address <= 0x3fff) {
        address &= 0x1f;

        if (address == 0x0010)
            address = 0x0000;
        if (address == 0x0014)
            address = 0x0004;
        if (address == 0x0018)
            address = 0x0008;
        if (address == 0x001C)
            address = 0x000C;

        bus->palette[address] = value;
        // GLOBAL_PALETTE[address] = value;
    }
}

uint8_t ppu_read_from_bus(Bus *bus, uint16_t address) {
    uint8_t value = 0;
    if (address <= 0x0fff) {
        value = bus->pattern_table_0[address];
    }

    else if (address <= 0x1fff) {
        address &= 0x0fff;
        value = bus->pattern_table_1[address];
    }

    else if (address <= 0x2fff) {
        if (bus->ppu->mirror_mode == SINGLE_SCREEN_LOWER) {
            address &= 0x03ff;
            value = bus->name_table_0[address];
        }

        else if (bus->ppu->mirror_mode == SINGLE_SCREEN_UPPER) {
            address &= 0x03ff;
            value = bus->name_table_1[address];
        }

        else {
            if (address <= 0x23ff) {
                address &= 0x03ff;
                value = bus->name_table_0[address];
            }

            else if (address <= 0x27ff) {
                address &= 0x03ff;
                if (bus->ppu->mirror_mode == HORIZONTAL)
                    value = bus->name_table_0[address];
                else if (bus->ppu->mirror_mode = VERTICAL)
                    value = bus->name_table_1[address];
            }

            else if (address <= 0x2bff) {
                address &= 0x03ff;
                if (bus->ppu->mirror_mode == HORIZONTAL)
                    value = bus->name_table_1[address];
                else if (bus->ppu->mirror_mode = VERTICAL)
                    value = bus->name_table_0[address];
            }

            else if (address <= 0x2fff) {
                address &= 0x03ff;
                value = bus->name_table_1[address];
            }
        }
    }

    else if (address >= 0x3f00 && address <= 0x3fff) {
        address &= 0x001f;

        if (address == 0x0010)
            address = 0x0000;
        if (address == 0x0014)
            address = 0x0004;
        if (address == 0x0018)
            address = 0x0008;
        if (address == 0x001C)
            address = 0x000C;

        value = bus->palette[address];
        // printf("PALETTE VALUE = %04x\n", address);
    }

    return value;
}

void clock_bus(Bus *bus, SDL_Window *window) {
    clock_ppu(bus->ppu, window);
    // printf("ppu->scanline = %d\n", bus->ppu->scanline);
    // printf("ppu->cycles = %d\n", bus->ppu->cycles);
    if (bus->system_cycles % 3 == 0 && !bus->ppu->oamdma_write) {
        clock_cpu(bus->cpu);
    }

    if (bus->ppu->status.vblank && bus->ppu->nmi && !bus->ppu->oamdma_write) {
        bus->ppu->nmi = false;
        nmi(bus->cpu);
    }

    if (bus->previous_vram_bank == 0 && bus->ppu->vram_address.reg >= 0x1000) {
        bus->previous_vram_bank = 1;
        bus->mapper->a12_rising_edge();
    }

    else if (bus->previous_vram_bank == 1 && bus->ppu->vram_address.reg < 0x1000) {
        bus->previous_vram_bank = 0;
    }

    bus->system_cycles++;
}

Bus *InitBus(void) {
    Bus *bus = (Bus *)malloc(sizeof(Bus));

    bus->cpu_ram = (uint8_t *)malloc(0x800);
    bus->ppu_registers = (uint8_t *)malloc(0x8);
    bus->apu_io_registers = (uint8_t *)malloc(0x18);
    bus->unmapped = (uint8_t *)malloc(0xBFE0);

    bus->pattern_table_0 = (uint8_t *)malloc(0x1000);
    bus->pattern_table_1 = (uint8_t *)malloc(0x1000);
    bus->name_table_0 = (uint8_t *)malloc(0x0400);
    bus->name_table_1 = (uint8_t *)malloc(0x0400);
    bus->name_table_2 = (uint8_t *)malloc(0x0400);
    bus->name_table_3 = (uint8_t *)malloc(0x0400);
    bus->palette = (uint8_t *)malloc(0x20);

    bus->mapper = NULL;
    bus->cpu = NULL;
    bus->ppu = NULL;

    bus->system_cycles = 0;

    bus->poll_input1 = 0;
    bus->poll_input2 = 0;

    return bus;
}