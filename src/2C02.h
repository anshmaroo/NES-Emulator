#include <stdint.h>
#include <stdbool.h>
#include "window.h"

typedef struct Control {
    bool nmi_enable;
    bool master_slave_select;
    bool sprite_height;
    bool background_tile_select;
    bool sprite_tile_select;
    bool vram_increment_mode;
    short nametable_select;

} Control;

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

typedef union loopy_register {
    struct  {
        uint16_t coarse_x : 5;
        uint16_t coarse_y : 5;
        uint16_t nametable_y : 1;
        uint16_t nametable_x : 1;
        uint16_t fine_y : 3; 
        uint16_t unused : 1;
    };

    uint16_t reg;
} loopy_register;

typedef struct State2C02 {
    Control control;
    Mask mask;
    Status status;
    OAMaddr oamaddr;
    OAMdata oamdata;
    PPUscroll ppuscroll;
    PPUaddr ppuaddr; 
    PPUdata ppudata;
    OAMdma oamdma;

    // INTERNAL REGISTERS
    loopy_register vram_address;
    loopy_register tram_address;
    uint8_t fine_x;
    bool w;


    // OAM
    uint8_t *primary_oam;      // SPRITE DATA (256 bytes)
    uint8_t *secondary_oam;    // SPRITE DATA (32 bytes)

    // OAMDMA
    bool oamdma_write;
    int oamdma_clock;

    
    // PPU STATUS
    int scanline;
    int cycles;

    uint8_t data_buffer;

    // background rendering
    uint8_t bg_next_tile_index;
    uint8_t bg_next_tile_attribute;
    uint8_t bg_next_tile_lsb;
    uint8_t bg_next_tile_msb;

    uint16_t bg_shifter_pattern_lo;
    uint16_t bg_shifter_pattern_hi;
    uint16_t bg_shifter_attribute_lo;
    uint16_t bg_shifter_attribute_hi;

    // sprite rendering
    uint8_t sprite_count;

    uint8_t primary_oam_address;
    uint8_t secondary_oam_address;

    uint8_t *sprite_shifter_pattern_lo;
    uint8_t *sprite_shifter_pattern_hi;

    bool nmi;

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


void write_to_primary_oam(State2C02 *ppu, uint8_t address, uint8_t value);

uint8_t read_from_primary_oam(State2C02 *ppu, uint8_t address);

void write_to_secondary_oam(State2C02 *ppu, uint8_t address, uint8_t value);

uint8_t read_from_secondary_oam(State2C02 *ppu, uint8_t address);


/**
 * @brief render pattern tables to window
 * 
 * @param window 
 */
void render_pattern_tables(State2C02 *ppu, SDL_Window *window);

/**
 * @brief render all 4 nametables
 * 
 * @param ppu 
 * @param window 
 */
void render_nametables(State2C02 *ppu, SDL_Window *window);

/**
 * @brief print nametables to file
 * 
 * @param ppu 
 */
void print_nametables(State2C02 *ppu);



/**
 * @brief increases vram x position and switches to next nametable if necessary
 * 
 * @param ppu 
 */
void increment_scroll_x(State2C02 * ppu);

/**
 * @brief increases vram y position and switches to next nametable if necessary
 * 
 * @param ppu 
 */
void increment_scroll_y(State2C02 * ppu);



/**
 * @brief transfer x address from tram to vram
 * 
 * @param ppu 
 */
void transfer_address_x(State2C02 *ppu);

/**
 * @brief transfer y address from tram to vram
 * 
 * @param ppu 
 */
void transfer_address_y(State2C02 *ppu);



/**
 * @brief loads background "shift registers"
 * 
 * @param ppu 
 */
void load_background_shifters(State2C02 *ppu);

/**
 * @brief shifts background "shift registers" by one bit
 * 
 * @param ppu 
 */
void update_background_shifters(State2C02 *ppu);


/**
 * @brief shifts sprite "shift registers" by one bit
 * 
 * @param ppu 
 */
void update_sprite_shifters(State2C02 *ppu);


/**
 * @brief execute one ppu cycle
 * 
 * @param ppu 
 */
void clock_ppu(State2C02 *ppu, SDL_Window *window);