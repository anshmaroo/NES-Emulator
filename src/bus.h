#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct Bus {
    // CPU ADDRESSES
    uint8_t *cpu_ram;           // $0000–$07FF, mirrored until $1FFF
    uint8_t *ppu_registers;     // $2000–$2007, mirrored until $3FFF
    uint8_t *apu_io_registers;  // $4000–$4017
    uint8_t *unmapped;          // $4020–$FFFF, generally used for cartridge ram, rom, and mapper registers

    // PPU ADDRESSES
    uint8_t *pattern_table_0;  // $0000-$0FFF
    uint8_t *pattern_table_1;  // $1000-$1FFF
    uint8_t *name_table_0;     // $2000-$23BF
    uint8_t *name_table_1;     // $2400-$27FF
    uint8_t *name_table_2;     // $2800-$2BFF
    uint8_t *name_table_3;     // $2C00-$2FFF
    uint8_t *palette;         // $3F00-$3F1F

    // DEVICES
    struct State6502 *cpu;
    struct State2C02 *ppu;
    struct Controller *controller_1;
    struct Controller *controller_2;


    // SYSTEM STATUS
    uint16_t system_cycles;
    int poll_input1;
    int poll_input2;

} Bus;

void cpu_write_to_bus(Bus *bus, uint16_t address, uint8_t value);

uint8_t cpu_read_from_bus(Bus *bus, uint16_t address);

void ppu_write_to_bus(Bus *bus, uint16_t address, uint8_t value);

uint8_t ppu_read_from_bus(Bus *bus, uint16_t address);

void clock_bus(Bus *bus, SDL_Window *window);

Bus *InitBus(void);