#include "bus.h"
#include "controller.h"
#include "2C02.h"
#include "6502.h"


const int GLOBAL_COLORS[0x40] = {
    0x626262, 0x001fb2, 0x2404c8, 0x5200b2, 0x730076, 0x800024, 0x730b00, 0x522800, 0x244400, 0x005700, 0x005700, 0x005700, 0x003C76, 0x000000, 0x000000, 0x000000,
    0xABABAB, 0x0D57FF, 0x4B30FF, 0x8A13FF, 0xBC08D6, 0xD21269, 0xC72E00, 0x9D5400, 0x607B00, 0x209800, 0x00A300, 0x009942, 0x007DB4, 0x000000, 0x000000, 0x000000,
    0xFFFFFF, 0x53AEFF, 0x9085FF, 0xD365FF, 0xFF57FF, 0xFF5DCF, 0xFF7757, 0xFA9E00, 0xBDC700, 0x7AE700, 0x43F611, 0x26EF7E, 0x2CD5F6, 0x4E4E4E, 0x000000, 0x000000
};

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
        else if(address == 0x4016) {
            bus->poll_input = (value) ? 0 : -1;
        }
        else {
            address &= 0x0017;
            bus->apu_io_registers[address] = value;
        }
    }

    else if (address >= 0x4020 && address <= 0xffff) {
        // UNALLOCATED (CARTRIDGE RAM/ROM)
        address -= 0x4020;
        bus->unmapped[address] = value;
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
            if(bus->poll_input >= 0) {
                uint8_t value = read_from_controller(bus->controller_1, bus->poll_input++);
                if(bus->poll_input > 7)
                    bus->poll_input = -1;
                value = value | 0x40; 
                // printf("VALUE: %02x\n", value);
            }
            value = 0x40;

        }
        else {
            address &= 0x0017;
            value = bus->apu_io_registers[address];
        }
    }

    else if (address >= 0x4020 && address <= 0xffff) {
        address -= 0x4020;
        // printf("effective address = %04x\n", address);
        // getchar();
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

    else if (address <= 0x23ff) {
        address &= 0x03ff;
        bus->name_table_0[address] = value;
    }

    else if (address <= 0x27ff) {
        address &= 0x03ff;
        bus->name_table_1[address] = value;
    }

    else if (address <= 0x2bff) {
        address &= 0x03ff;
        bus->name_table_2[address] = value;
    }

    else if (address <= 0x2fff) {
        address &= 0x03ff;
        bus->name_table_3[address] = value;
    }

    else if (address >= 0x3f00 && address <= 0x3fff) {
        address &= 0x001f;
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

    else if (address <= 0x23ff) {
        address &= 0x03ff;
        value = bus->name_table_0[address];
    }

    else if (address <= 0x27ff) {
        address &= 0x03ff;
        value = bus->name_table_1[address];
    }

    else if (address <= 0x2bff) {
        address &= 0x03ff;
        value = bus->name_table_2[address];
    }

    else if (address <= 0x2fff) {
        address &= 0x03ff;
        value = bus->name_table_3[address];
    }

    else if (address >= 0x3f00 && address <= 0x3fff) {
        address &= 0x003f;
        value = GLOBAL_COLORS[address];
    }

    return value;
}

void clock_bus(Bus *bus, SDL_Window *window) {
    clock_ppu(bus->ppu, window);
    // printf("ppu->scanline = %d\n", bus->ppu->scanline);
    // printf("ppu->cycles = %d\n", bus->ppu->cycles);
    if (bus->system_cycles % 3 == 0) {
        clock_cpu(bus->cpu);
    }

    if (bus->ppu->nmi) {
        bus->ppu->nmi = false;
        nmi(bus->cpu);
        
    }

    bus->system_cycles++;
}

Bus *InitBus(void) {
    
    Bus *bus = malloc(sizeof(Bus));

    bus->cpu_ram = malloc(0x0800);
    bus->ppu_registers = malloc(0x8);
    bus->apu_io_registers = malloc(0x18);
    bus->unmapped = malloc(0xBFE0);

    bus->pattern_table_0 = malloc(0x1000);
    bus->pattern_table_1 = malloc(0x1000);
    bus->name_table_0 = malloc(0x0400);
    bus->name_table_1 = malloc(0x0400);
    bus->name_table_2 = malloc(0x0400);
    bus->name_table_3 = malloc(0x0400);
    

    bus->cpu = NULL;
    bus->ppu = NULL;

    bus->system_cycles = 0;
    bus->poll_input = 0;

    return bus;
}