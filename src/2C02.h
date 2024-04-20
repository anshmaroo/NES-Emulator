#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "window.h"

typedef struct Controller {
    bool nmi_enable;
    bool master_slave_select;
    bool sprite_height;
    bool background_tile_select;
    bool sprite_tile_select;
    bool vram_increment_mode;
    short nametable_select;

} Controller;

typedef struct Mask {
    bool blue;
    bool green;
    bool red;
    bool sprite_enable;
    bool background_enable;
    bool sprite_left_column_enable;
    bool background_left_column_enable;
    bool grayscale;
} Mask;

typedef struct Status {
    bool vblank;
    bool sprite_0_hit;
    bool sprite_overflow;
} Status;

typedef struct OAMaddr {
    uint8_t address;
} OAMaddr;

typedef struct OAMdata {
    uint8_t data;
} OAMdata;

typedef struct PPUscroll {
    uint8_t scroll;
} PPUscroll;

typedef struct PPUaddr {
    uint8_t address_byte;
} PPUaddr;

typedef struct PPUdata {
    uint8_t data;
} PPUdata;

typedef struct OAMdma {
    uint8_t address_high_byte;
} OAMdma;

typedef struct State2C02 {
    Controller controller;
    Mask mask;
    Status status;
    OAMaddr oamaddr;
    OAMdata oamdata;
    PPUscroll ppuscroll;
    PPUaddr ppuaddr; 
    PPUdata ppudata;
    OAMdma oamdma;

    
    // PPU STATUS
    uint16_t scanline;
    uint16_t cycles;
    uint8_t data_buffer;
    uint16_t address;
    bool address_latch;

    // BUS
    struct Bus *bus;

} State2C02;

/**
 * @brief creates 2C02 object
 * 
 */
State2C02 *Init2C02();

/**
 * @brief update ppu cycles
 * 
 * @param ppu 
 * @param count 
 */
void ppu_add_cycles(State2C02 *ppu, uint8_t count);

/**
 * @brief write to ppu register
 * 
 * @param ppu 
 * @param address 
 * @param value 
 */
void write_to_ppu_register(State2C02 *ppu, uint16_t address, uint8_t value);

/**
 * @brief read from ppu register
 * 
 * @param ppu 
 * @param address 
 * @return uint8_t 
 */
uint8_t read_from_ppu_register(State2C02 *ppu, uint16_t address);

/**
 * @brief render pattern tables to window
 * 
 * @param window 
 */
void render_pattern_tables(State2C02 *ppu, SDL_Window *window);