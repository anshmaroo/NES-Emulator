#include "2C02.h"
#include "bus.h"
#include <stdio.h>

const int SYSTEM_PALETTE[0x40] = {
    0x626262, 0x001FB2, 0x2404C8, 0x5200B2, 0x730076, 0x800024, 0x730B00, 0x522800, 0x244400, 0x005700, 0x005C00, 0x005324, 0x003C76, 0x000000, 0x000000, 0x000000, 
    0xABABAB, 0x0D57FF, 0x4B30FF, 0x8A13FF, 0xBC08D6, 0xD21269, 0xC72E00, 0x9D5400, 0x607B00, 0x209800, 0x00A300, 0x009942, 0x007DB4, 0x000000, 0x000000, 0x000000, 
    0xFFFFFF, 0x53AEFF, 0x9085FF, 0xD365FF, 0xFF57FF, 0xFF5DCF, 0xFF7757, 0xFA9E00, 0xBDC700, 0x7AE700, 0x43F611, 0x26EF7E, 0x2CD5F6, 0x4E4E4E, 0x000000, 0x000000, 
    0xFFFFFF, 0xB6E1FF, 0xCED1FF, 0xE9C3FF, 0xFFBCFF, 0xFFBDF4, 0xFFC6C3, 0xFFD59A, 0xE9E681, 0xCEF481, 0xB6FB9A, 0xA9FAC3, 0xA9F0F4, 0xB8B8B8, 0x000000, 0x000000

};

State2C02 *Init2C02() {
    State2C02 *state = malloc(sizeof(State2C02));

    state->bus = NULL;
    state->cycles = 0;

    return state;
}

void ppu_add_cycles(State2C02 *ppu, uint8_t count) {
    ppu->cycles += count;
}

/**
 * @brief write to ppu register
 *
 * @param ppu
 * @param address
 * @param value
 */
void write_to_ppu_register(State2C02 *ppu, uint16_t address, uint8_t value) {
    switch (address) {
        case 0x00:  // control
            ppu->control.nmi_enable = ((value) >> 7) & 0x1;
            ppu->control.master_slave_select = ((value) >> 6) & 0x1;
            ppu->control.sprite_height = ((value) >> 5) & 0x1;
            ppu->control.background_tile_select = ((value) >> 4) & 0x1;
            ppu->control.sprite_tile_select = ((value) >> 3) & 0x1;
            ppu->control.vram_increment_mode = ((value) >> 2) & 0x1;
            ppu->control.nametable_select = (((value) >> 1) & 0x1) | (value & 0x1);
            break;

        case 0x01:  // MASK
            ppu->mask.blue = ((value) >> 7) & 0x1;
            ppu->mask.green = ((value) >> 6) & 0x1;
            ppu->mask.red = ((value) >> 5) & 0x1;
            ppu->mask.sprite_enable = ((value) >> 4) & 0x1;
            ppu->mask.background_enable = ((value) >> 3) & 0x1;
            ppu->mask.sprite_left_column_enable = ((value) >> 2) & 0x1;
            ppu->mask.background_left_column_enable = ((value) >> 1) & 0x1;
            ppu->mask.grayscale = value & 0x1;
            break;

        case 0x02:  // STATUS
            ppu->status.vblank = ((value) >> 7) & 0x1;
            ppu->status.sprite_0_hit = ((value) >> 6) & 0x1;
            ppu->status.sprite_overflow = ((value) >> 5) & 0x1;
            break;

        case 0x03:  // OAMADDR
            ppu->oamaddr.address = value;
            break;

        case 0x04:  // OAMDATA
            ppu->oamdata.data = value;
            break;

        case 0x05:  // PPUSCROLL
            ppu->ppuscroll.scroll = value;
            break;

        case 0x06:  // PPUADDR
            // getchar();

            if (ppu->address_latch == 0) {
                ppu->address = (value << 8) & 0x3f00;
                ppu->address_latch = 1;
            }

            else if (ppu->address_latch == 1) {
                ppu->address |= value;

                // getchar();
                ppu->address_latch = 0;
            }

            break;

        case 0x07:  // PPUDATA
            ppu->ppudata.data = value;
            ppu_write_to_bus(ppu->bus, ppu->address, value);
            // if(ppu->address >= 0x3f00 && ppu->address <= 0x3f1f) {
            //     printf("WROTE $%02x TO ADDRESS $%04X\n", value, ppu->address);
            // }

            ppu->address += (ppu->control.vram_increment_mode == 1) ? 32 : 1;
            ppu->address &= 0x3fff;
            break;

        case 0x4014:  // OAMDMA
            ppu->oamdma.address_high_byte = value;
            break;
    }
}

/**
 * @brief read from ppu register
 *
 * @param ppu
 * @param address
 * @return uint8_t
 */
uint8_t read_from_ppu_register(State2C02 *ppu, uint16_t address) {
    uint8_t value = 0;
    switch (address) {
        case 0x00:  // control
            value |= ppu->control.nmi_enable << 7;
            value |= ppu->control.master_slave_select << 6;
            value |= ppu->control.sprite_height << 5;
            value |= ppu->control.background_tile_select << 4;
            value |= ppu->control.sprite_tile_select << 3;
            value |= ppu->control.vram_increment_mode << 2;
            value |= ppu->control.nametable_select;

            if (ppu->status.vblank && ppu->control.nmi_enable)
                ppu->nmi = true;

            break;

        case 0x01:  // MASK
            value |= ppu->mask.blue << 7;
            value |= ppu->mask.green << 6;
            value |= ppu->mask.red << 5;
            value |= ppu->mask.sprite_enable << 4;
            value |= ppu->mask.background_enable << 3;
            value |= ppu->mask.sprite_left_column_enable << 2;
            value |= ppu->mask.background_left_column_enable << 1;
            value |= ppu->mask.grayscale;
            break;

        case 0x02:  // STATUS
            value |= ppu->status.vblank << 7;
            value |= ppu->status.sprite_0_hit << 6;
            value |= ppu->status.sprite_overflow << 5;
            value |= ppu->data_buffer & 0x1f;

            ppu->status.vblank = 0;
            ppu->address_latch = 0;
            break;

        case 0x03:  // OAMADDR
            value = ppu->oamaddr.address;
            break;

        case 0x04:  // OAMDATA
            value = ppu->oamdata.data;
            break;

        case 0x05:  // PPUSCROLL
            value = ppu->ppuscroll.scroll;
            break;

        case 0x06:  // PPUADDR
            break;

        case 0x07:  // PPUDATA
            value = ppu->data_buffer;
            ppu->data_buffer = ppu_read_from_bus(ppu->bus, ppu->address);

            if (ppu->address >= 0x3f00)
                value = ppu_read_from_bus(ppu->bus, ppu->address);

            ppu->address += (ppu->control.vram_increment_mode == 1) ? 32 : 1;
            ppu->address &= 0x3fff;

            break;

        case 0x4014:  // OAMDMA
            value = ppu->oamdma.address_high_byte;
            break;
    }

    return value;
}

/**
 * @brief render pattern tables to window
 *
 * @param ppu
 * @param window
 */
void render_pattern_tables(State2C02 *ppu, SDL_Window *window) {
    int width = SDL_GetWindowSurface(window)->w;
    // int height = SDL_GetWindowSurface(window)->h;

    uint32_t colors[4] = {0x00000, 0x555555, 0xbbbbbb, 0xffffff};

    for (int tile_address = 0; tile_address < 0x1000; tile_address += 16) {
        for (int byte = tile_address; byte < tile_address + 8; byte++) {
            for (int bit = 0; bit < 8; bit++) {
                uint8_t pixel = ppu_read_from_bus(ppu->bus, byte) >> (7 - (bit % 8)) & 1;
                pixel += (ppu_read_from_bus(ppu->bus, byte + 8) >> (7 - (bit % 8)) & 1) * 2;
                int tile_x = (tile_address / 16) % (width / 8);        // Tile's x position within the array
                int tile_y = (tile_address / (16 * (width / 8))) * 8;  // Tile's y position within the array
                int x = tile_x * 8 + bit;                              // Calculate the x coordinate within the tile
                int y = tile_y + (byte % 8);                           // Calculate the y coordinate within the tile

                set_pixel(window, x, y, colors[pixel]);
            }
        }
    }

    SDL_UpdateWindowSurface(window);
}

/**
 * @brief render all 4 nametables
 *
 * @param ppu
 * @param window
 */
void render_nametables(State2C02 *ppu, SDL_Window *window) {
    uint8_t scale = SDL_GetWindowSurface(window)->w / 256;
    // uint32_t colors[4] = {0x000000, 0x444444, 0x999999, 0xffffff};

    uint16_t nametable_start = 0x2000 + ppu->control.nametable_select * 0x400;
    uint16_t attribute_table_start = nametable_start + 0x400 - 0x40;
    uint16_t pattern_table_start = ppu->control.background_tile_select * 0x1000;

    // for(int i = 0x3f00; i < 0x3f10; i++)
    //     printf("$%04x - $%02x ", i, ppu_read_from_bus(ppu->bus, i));

    // printf("\n");

    for (int index = nametable_start; index < nametable_start + 0x400 - 0x40; index++) {
        uint16_t tile_address = ppu_read_from_bus(ppu->bus, index) * 0x10 + pattern_table_start;
        int tile_x = (index % 32);             // Tile's x position within the array
        int tile_y = ((index - 0x2000) / 32);  // Tile's y position within the array
        for (int byte = tile_address; byte < tile_address + 8; byte++) {

            uint16_t attribute_table_address = attribute_table_start + (tile_x / 4) + ((tile_y / 4) * 8);
            uint8_t attribute_byte = ppu_read_from_bus(ppu->bus, attribute_table_address);
            uint8_t palette = (attribute_byte >> ((tile_x & 2) + ((tile_y & 2) * 2))) & 0x3;

            for (int bit = 0; bit < 8; bit++) {
                uint8_t pixel = ppu_read_from_bus(ppu->bus, byte) >> (7 - (bit % 8)) & 1;
                pixel += ((ppu_read_from_bus(ppu->bus, byte + 8) >> (7 - (bit % 8)) & 1) * 2);
                
                uint16_t system_palette_index = ppu_read_from_bus(ppu->bus, 0x3f00 + (palette << 2) + pixel);

                // get pixel positions in the scaled tile
                int x_start = tile_x * 8 * scale + bit * scale;
                int y_start = tile_y * 8 * scale + (byte % 8) * scale;

                int x = x_start;
                int y = y_start;

                // Render each scaled pixel
                for (int i = 0; i < scale * scale; i++) {
                    x = x_start + (i % scale);
                    y = y_start + (i / scale);

                    set_pixel(window, x, y, SYSTEM_PALETTE[system_palette_index]);
                }
            }
        }
    }

    SDL_UpdateWindowSurface(window);
}

/**
 * @brief print nametables to file
 *
 * @param ppu
 */
void print_nametables(State2C02 *ppu) {
    FILE *d = fopen("nametable.txt", "w+");

    int nametable_start = 0x2000 + ppu->control.nametable_select * 0x400;
    for (int i = nametable_start; i < nametable_start + 0x400; i++) {
        if (i % 32 == 0 && i > 0)
            fprintf(d, "\n");
        fprintf(d, "%02x ", ppu_read_from_bus(ppu->bus, i));
    }

    fclose(d);
}

/**
 * @brief execute one ppu cycle
 *
 * @param ppu
 */
void clock_ppu(State2C02 *ppu, SDL_Window *window) {
    ppu->cycles++;

    if (ppu->cycles > 341) {
        if (ppu->scanline == 262) {
            ppu->scanline = 0;
        }

        else {
            ppu->scanline++;
        }
        ppu->cycles = 0;
    }

    if (ppu->scanline >= 0 && ppu->scanline <= 239) {
        // fetch nametable, attribute table, and pattern table bytes
        if (ppu->cycles >= 1 && ppu->cycles <= 256) {
        }
    }

    if (ppu->scanline == 241 && ppu->cycles == 1) {
        ppu->status.vblank = 1;
        if (ppu->control.nmi_enable)
            ppu->nmi = true;
        // printf("NMI!\n");

        // print_nametables(ppu);
        render_nametables(ppu, window);
        SDL_Delay(16);
    }

    if (ppu->scanline == 261 && ppu->cycles == 1) {
        ppu->status.vblank = 0;
    }
}