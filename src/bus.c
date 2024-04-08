#include "bus.h"
#include "2C02.h"
#include "6502.h"

void cpu_write_to_bus(Bus *bus, uint16_t address, uint8_t value) {
    if (address <= 0x1fff) {
        // CPU RAM
        address &= 0x07ff;
        bus->cpu_ram[address] = value;
    } 
    
    else if (address <= 0x3fff) {
        // PPU REGISTERS
        address &= 0x0007;
        write_to_ppu_register(bus->ppu, address, value);
    } 
    
    else if (address <= 0x4017) {
        // APU/IO REGISTERS
        if (address == 0x4014)
            write_to_ppu_register(bus->ppu, address, value);
        else {
            address &= 0x0017;
            bus->apu_io_registers[address] = value;
        }
    } 
    
    else if (address >= 0x4020 && address <= 0xffff) {
        // UNALLOCATED (CARTRIDGE RAM/ROM)
        address &= 0xBFDF;
        bus->unmapped[address] = value;
    }
}

uint8_t cpu_read_from_bus(Bus *bus, uint16_t address) {
    uint8_t value = 0;
    if (address <= 0x1fff) {
        address &= 0x07ff;
        value = bus->cpu_ram[address];
    } else if (address <= 0x3fff) {
        address &= 0x0007;

        switch (address) {
            read_from_ppu_register(bus->ppu, address);
        }

    } else if (address <= 0x4017) {
        if (address == 0x4014)
            read_from_ppu_register(bus->ppu, address);
        else {
            address &= 0x0017;
            value = bus->apu_io_registers[address];
        }
    } else if (address >= 0x4020 && address <= 0xffff) {
        address &= 0xBFDF;
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
        bus->palette[address] = value;
    }
}

uint8_t ppu_read_from_bus(Bus *bus, uint16_t address) {
    uint8_t value = 0;
    if (address <= 0x0fff) {
        value = bus->pattern_table_0[address];
    } else if (address <= 0x1fff) {
        address &= 0x0fff;
        value = bus->pattern_table_1[address];
    } else if (address <= 0x23bf) {
        address &= 0x03ff;
        value = bus->name_table_0[address];
    } else if (address <= 0x27ff) {
        address &= 0x03ff;
        value = bus->name_table_1[address];
    }else if (address <= 0x2bff) {
        address &= 0x03ff;
        value = bus->name_table_2[address];
    } else if (address <= 0x2fff) {
        address &= 0x03ff;
        value = bus->name_table_3[address];
    } else if (address >= 0x3f00 && address <= 0x3fff) {
        address &= 0x001f;
        value = bus->palette[address];
    }

    return value;
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
    bus->palette = malloc(0x0020);

    bus->cpu = NULL;
    bus->ppu = NULL;

    return bus;
}