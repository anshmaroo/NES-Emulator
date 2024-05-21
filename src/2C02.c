#include "2C02.h"

#include <stdio.h>

#include "bus.h"

const int SYSTEM_PALETTE[0x40] = {
    0x626262, 0x001FB2, 0x2404C8, 0x5200B2, 0x730076, 0x800024, 0x730B00, 0x522800, 0x244400, 0x005700, 0x005C00, 0x005324, 0x003C76, 0x000000, 0x000000, 0x000000,
    0xABABAB, 0x0D57FF, 0x4B30FF, 0x8A13FF, 0xBC08D6, 0xD21269, 0xC72E00, 0x9D5400, 0x607B00, 0x209800, 0x00A300, 0x009942, 0x007DB4, 0x000000, 0x000000, 0x000000,
    0xFFFFFF, 0x53AEFF, 0x9085FF, 0xD365FF, 0xFF57FF, 0xFF5DCF, 0xFF7757, 0xFA9E00, 0xBDC700, 0x7AE700, 0x43F611, 0x26EF7E, 0x2CD5F6, 0x4E4E4E, 0x000000, 0x000000,
    0xFFFFFF, 0xB6E1FF, 0xCED1FF, 0xE9C3FF, 0xFFBCFF, 0xFFBDF4, 0xFFC6C3, 0xFFD59A, 0xE9E681, 0xCEF481, 0xB6FB9A, 0xA9FAC3, 0xA9F0F4, 0xB8B8B8, 0x000000, 0x000000

};

State2C02 *Init2C02() {
    State2C02 *state = malloc(sizeof(State2C02));

    state->primary_oam = (Sprite *)malloc(sizeof(Sprite) * 64);
    state->secondary_oam = (Sprite *)malloc(sizeof(Sprite) * 8);

    for (int i = 0; i < 64; i++) {
        state->primary_oam[i].y = 0;
        state->primary_oam[i].tile_index = 0;
        state->primary_oam[i].attributes = 0;
        state->primary_oam[i].x = 0;
        if (i % 8 == 0) {
            state->secondary_oam[i / 8].y = 0;
            state->secondary_oam[i / 8].tile_index = 0;
            state->secondary_oam[i / 8].attributes = 0;
            state->secondary_oam[i / 8].x = 0;
        }
    }

    state->vram_address.reg = 0;
    state->tram_address.reg = 0;
    state->sprite_shifter_pattern_lo = malloc(0x8);
    state->sprite_shifter_pattern_hi = malloc(0x8);

    state->bus = NULL;
    state->cycles = 0;
    state->scanline = 0;

    state->oamdma_clock = 0;

    return state;
}

/**
 * @brief flips a byte horizontally
 *
 * @param byte
 * @return uint8_t
 */
uint8_t flip_byte(uint8_t byte) {
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((byte >> i) & 1) << (7 - i);
    }
    return result;
}

/**
 * @brief write to ppu register
 *
 * @param ppu
 * @param address
 * @param value
 */
void write_to_ppu_register(State2C02 *ppu, uint16_t address, uint8_t value) {
    ppu->io_db = value;
    switch (address) {
        case 0x00:  // control
            ppu->control.nmi_enable = ((value) >> 7) & 0x1;
            ppu->control.master_slave_select = ((value) >> 6) & 0x1;
            ppu->control.sprite_height = ((value) >> 5) & 0x1;
            ppu->control.background_tile_select = ((value) >> 4) & 0x1;
            ppu->control.sprite_tile_select = ((value) >> 3) & 0x1;
            ppu->control.vram_increment_mode = ((value) >> 2) & 0x1;
            ppu->control.nametable_select = (value & 0x3);

            ppu->tram_address.nametable_y = (ppu->control.nametable_select & 0x2) >> 1;
            ppu->tram_address.nametable_x = ppu->control.nametable_select & 0x1;

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
            ppu->status.sprite_zero_hit = ((value) >> 6) & 0x1;
            ppu->status.sprite_overflow = ((value) >> 5) & 0x1;
            break;

        case 0x03:  // OAMADDR
            ppu->oamaddr.address = value;
            break;

        case 0x04:  // OAMDATA
            switch ((ppu->oamaddr.address % 4)) {
                case 0:
                    ppu->primary_oam[ppu->oamaddr.address / 4].y = value;
                    break;
                case 1:
                    ppu->primary_oam[ppu->oamaddr.address / 4].tile_index = value;
                    break;
                case 2:
                    ppu->primary_oam[ppu->oamaddr.address / 4].attributes = value;
                    break;
                case 3:
                    ppu->primary_oam[ppu->oamaddr.address / 4].x = value;
                    break;

                    // printf("WRITING SPRITE!\n");
            }

            ppu->oamdata.data = value;
            ppu->oamaddr.address++;

            break;

        case 0x05:  // PPUSCROLL
            if (ppu->w == 0) {
                ppu->fine_x = value & 0x7;                // up to 8 pixel scroll
                ppu->tram_address.coarse_x = value >> 3;  // tile scroll

                ppu->w = 1;
            }

            else if (ppu->w == 1) {
                ppu->tram_address.fine_y = value & 0x7;
                ppu->tram_address.coarse_y = value >> 3;

                ppu->w = 0;
            }
            break;

        case 0x06:  // PPUADDR

            if (ppu->w == 0) {
                ppu->tram_address.reg = (uint16_t)((value & 0x3F) << 8) | (ppu->tram_address.reg & 0x00FF);
                ppu->w = 1;
            }

            else if (ppu->w == 1) {
                ppu->tram_address.reg = (ppu->tram_address.reg & 0xFF00) | value;
                ppu->vram_address = ppu->tram_address;
                ppu->w = 0;
            }

            break;

        case 0x07:  // PPUDATA
            ppu->ppudata.data = value;

            ppu_write_to_bus(ppu->bus, ppu->vram_address.reg, value);

            ppu->vram_address.reg += ppu->control.vram_increment_mode ? 32 : 1;
            ppu->vram_address.reg &= 0x3fff;

            break;

        case 0x4014:  // OAMDMA
            ppu->oamdma.address_high_byte = value;
            ppu->oamdma_write = true;
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
            break;

        case 0x01:  // MASK
            break;

        case 0x02:  // STATUS
            value |= ppu->status.vblank << 7;
            value |= ppu->status.sprite_zero_hit << 6;
            value |= ppu->status.sprite_overflow << 5;
            value |= ppu->data_buffer & 0x1f;

            ppu->status.vblank = 0;
            ppu->w = 0;

            break;

        case 0x03:  // OAMADDR
            break;

        case 0x04:  // OAMDATA
            value = ppu->oamdata.data;
            break;

        case 0x05:  // PPUSCROLL
            break;

        case 0x06:  // PPUADDR
            break;

        case 0x07:  // PPUDATA
            value = ppu->data_buffer;
            ppu->data_buffer = ppu_read_from_bus(ppu->bus, ppu->vram_address.reg);
            if (ppu->vram_address.reg >= 0x3f00)
                value = ppu_read_from_bus(ppu->bus, ppu->vram_address.reg);
            ppu->vram_address.reg += (ppu->control.vram_increment_mode == 1) ? 32 : 1;
            ppu->vram_address.reg &= 0x3fff;
            break;
    }

    ppu->io_db = value;
    return value;
}

/**
 * @brief set the mirror mode
 *
 * @param ppu
 * @param mirror_mode
 */
void set_mirror_mode(State2C02 *ppu, bool mirror_mode) {
    ppu->mirror_mode = mirror_mode;
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

    uint16_t nametable_start = 0x2400;
    uint16_t attribute_table_start = nametable_start + 0x400 - 0x40;
    uint16_t pattern_table_start = ppu->control.background_tile_select * 0x1000;

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

    int nametable_start = ppu->vram_address.nametable_y * 0x800 + ppu->vram_address.nametable_x * 0x400 + 0x2000;
    for (int i = nametable_start; i < nametable_start + 0x400; i++) {
        if (i % 32 == 0 && i > 0)
            fprintf(d, "\n");
        fprintf(d, "%02x ", ppu_read_from_bus(ppu->bus, i));
    }

    fclose(d);
}

/**
 * @brief increases vram x position and switches to next nametable if necessary
 *
 * @param ppu
 */
void increment_scroll_x(State2C02 *ppu) {
    if (ppu->mask.background_enable || ppu->mask.sprite_enable) {
        if (ppu->vram_address.coarse_x == 31) {
            ppu->vram_address.coarse_x = 0;
            ppu->vram_address.nametable_x = !ppu->vram_address.nametable_x;
        }

        else {
            ppu->vram_address.coarse_x++;
        }
    }
}

/**
 * @brief increases vram y position and switches to next nametable if necessary
 *
 * @param ppu
 */
void increment_scroll_y(State2C02 *ppu) {
    if (ppu->mask.background_enable || ppu->mask.sprite_enable) {
        if (ppu->vram_address.fine_y < 7) {
            ppu->vram_address.fine_y++;
        } else {
            ppu->vram_address.fine_y = 0;

            if (ppu->vram_address.coarse_y == 29) {
                ppu->vram_address.coarse_y = 0;
                ppu->vram_address.nametable_y = !ppu->vram_address.nametable_y;
            }

            else if (ppu->vram_address.coarse_y == 31) {
                ppu->vram_address.coarse_y = 0;
            }

            else {
                ppu->vram_address.coarse_y++;
            }
        }
    }
}

/**
 * @brief transfer x address and nametable from tram to vram
 *
 * @param ppu
 */
void transfer_address_x(State2C02 *ppu) {
    if (ppu->mask.background_enable || ppu->mask.sprite_enable) {
        ppu->vram_address.nametable_x = ppu->tram_address.nametable_x;
        ppu->vram_address.coarse_x = ppu->tram_address.coarse_x;
    }
}

/**
 * @brief transfer y address from tram to vram
 *
 * @param ppu
 */
void transfer_address_y(State2C02 *ppu) {
    if (ppu->mask.background_enable || ppu->mask.sprite_enable) {
        ppu->vram_address.fine_y = ppu->tram_address.fine_y;
        ppu->vram_address.nametable_y = ppu->tram_address.nametable_y;
        ppu->vram_address.coarse_y = ppu->tram_address.coarse_y;
    }
}

/**
 * @brief loads background "shift registers"
 *
 * @param ppu
 */
void load_background_shifters(State2C02 *ppu) {
    ppu->bg_shifter_pattern_lo = (ppu->bg_shifter_pattern_lo & 0xFF00) | ppu->bg_next_tile_lsb;
    ppu->bg_shifter_pattern_hi = (ppu->bg_shifter_pattern_hi & 0xFF00) | ppu->bg_next_tile_msb;

    ppu->bg_shifter_attribute_lo = (ppu->bg_shifter_attribute_lo & 0xFF00) | ((ppu->bg_next_tile_attribute & 0b01) ? 0xFF : 0x00);
    ppu->bg_shifter_attribute_hi = (ppu->bg_shifter_attribute_hi & 0xFF00) | ((ppu->bg_next_tile_attribute & 0b10) ? 0xFF : 0x00);
}

/**
 * @brief shifts background "shift registers" by one bit
 *
 * @param ppu
 */
void update_background_shifters(State2C02 *ppu) {
    if (ppu->mask.background_enable) {
        ppu->bg_shifter_pattern_lo <<= 1;
        ppu->bg_shifter_pattern_hi <<= 1;
        ppu->bg_shifter_attribute_lo <<= 1;
        ppu->bg_shifter_attribute_hi <<= 1;
    }
}

/**
 * @brief shifts sprite "shift registers" by one bit
 *
 * @param ppu
 */
void update_sprite_shifters(State2C02 *ppu) {
    if (ppu->mask.sprite_enable && ppu->cycles >= 1 && ppu->cycles < 256) {
        for (int i = 0; i < ppu->sprite_count; i++) {
            if (ppu->secondary_oam[i].x > 0) {
                ppu->secondary_oam[i].x--;

            }

            else {
                ppu->sprite_shifter_pattern_lo[i] <<= 1;
                ppu->sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }
}

/**
 *
 * @brief execute one ppu cycle
 *
 * @param ppu
 */
void clock_ppu(State2C02 *ppu, SDL_Window *window) {
    // OAM DMA
    if (ppu->oamdma_write && ppu->cycles % 3 == 0) {
        // read (do nothing)
        if (ppu->oamdma_clock % 2 == 0)
            ;

        // write
        else {
            // printf("WRITING SPRITE!\n");
            uint16_t address = (ppu->oamdma.address_high_byte << 8) | (ppu->oamdma_clock / 2);
            uint8_t value = cpu_read_from_bus(ppu->bus, address);
            switch (ppu->oamdma_clock % 8) {
                case 1:
                    ppu->primary_oam[ppu->oamaddr.address / 4].y = value;
                    break;

                case 3:
                    ppu->primary_oam[ppu->oamaddr.address / 4].tile_index = value;
                    break;

                case 5:
                    ppu->primary_oam[ppu->oamaddr.address / 4].attributes = value;
                    break;

                case 7:
                    ppu->primary_oam[ppu->oamaddr.address / 4].x = value;
                    break;
            }
            ppu->oamaddr.address++;
        }

        if (ppu->oamdma_clock == 511) {
            ppu->oamdma_clock = 0;
            ppu->oamdma_write = false;
        }

        else {
            ppu->oamdma_clock++;
        }
    }

    // load backgrounds and sprites for the current scanline
    if (ppu->scanline >= -1 && ppu->scanline < 240) {
        // odd cycle switch
        if (ppu->scanline == 0 && ppu->cycles == 0) {
            ppu->cycles = 1;
        }

        // vblank
        if (ppu->scanline == -1 && ppu->cycles == 1) {
            ppu->status.vblank = 0;
            ppu->status.sprite_overflow = 0;
            ppu->status.sprite_zero_hit = 0;
        }

        // background graphics
        if ((ppu->cycles >= 2 && ppu->cycles < 258) || (ppu->cycles >= 321 && ppu->cycles < 338)) {
            // shift to next pixel
            update_background_shifters(ppu);
            update_sprite_shifters(ppu);

            switch ((ppu->cycles - 1) % 8) {
                case 0:
                    // start of new pixel, start next pixel
                    load_background_shifters(ppu);

                    // get index of pattern table
                    ppu->bg_next_tile_index = ppu_read_from_bus(ppu->bus, 0x2000 | (ppu->vram_address.reg & 0x0fff));
                    break;

                case 2:
                    // get attribute table byte for current pixel
                    ppu->bg_next_tile_attribute = ppu_read_from_bus(ppu->bus, 0x23C0 | (ppu->vram_address.nametable_y << 11) | (ppu->vram_address.nametable_x << 10) | ((ppu->vram_address.coarse_y >> 2) << 3) | (ppu->vram_address.coarse_x >> 2));

                    if (ppu->vram_address.coarse_y & 0x2)
                        ppu->bg_next_tile_attribute >>= 4;
                    if (ppu->vram_address.coarse_x & 0x2)
                        ppu->bg_next_tile_attribute >>= 2;

                    ppu->bg_next_tile_attribute &= 0x3;

                    break;

                case 4:
                    // get pattern table byte low for current pixel
                    ppu->bg_next_tile_lsb = ppu_read_from_bus(ppu->bus, (ppu->control.background_tile_select * 0x1000) | ((uint16_t)ppu->bg_next_tile_index * 0x10) | (ppu->vram_address.fine_y));
                    break;

                case 6:
                    // get pattern table byte high for current pixel
                    ppu->bg_next_tile_msb = ppu_read_from_bus(ppu->bus, (ppu->control.background_tile_select * 0x1000) | ((uint16_t)ppu->bg_next_tile_index * 0x10) | (ppu->vram_address.fine_y) + 8);
                    break;

                case 7:
                    // increment x scroll. this should also increase vram address
                    increment_scroll_x(ppu);
                    break;
            }
        }

        // pixel/tile select/scrolling
        if (ppu->cycles == 256) {
            // increment y scroll @ end of scanline. this should also increase vram address
            increment_scroll_y(ppu);
        }

        if (ppu->cycles == 257) {
            // reset x position
            load_background_shifters(ppu);
            transfer_address_x(ppu);
        }

        // load secondary OAM
        if (ppu->cycles == 257 && ppu->scanline >= 0) {
            uint8_t n = 0;
            ppu->sprite_zero_on_scanline = false;
            ppu->sprite_count = 0;

            for (int i = 0; i < 8; i++) {
                ppu->sprite_shifter_pattern_lo[i] = 0;
                ppu->sprite_shifter_pattern_hi[i] = 0;
            }

            while (n < 64 && ppu->sprite_count < 9) {
                // check if sprite is in a valid y range
                int difference = ppu->scanline - ppu->primary_oam[n].y;
                if (difference >= 0 && difference < (ppu->control.sprite_height ? 16 : 8)) {
                    if (ppu->sprite_count < 8) {
                        ppu->secondary_oam[ppu->sprite_count].y = ppu->primary_oam[n].y;
                        ppu->secondary_oam[ppu->sprite_count].tile_index = ppu->primary_oam[n].tile_index;
                        ppu->secondary_oam[ppu->sprite_count].attributes = ppu->primary_oam[n].attributes;
                        ppu->secondary_oam[ppu->sprite_count].x = ppu->primary_oam[n].x;

                        // increment sprite count
                        ppu->sprite_count++;

                        if (n == 0) {
                            ppu->sprite_zero_on_scanline = true;
                        }
                    }
                }
                // increment primary oam pointer
                n++;
            }

            ppu->status.sprite_overflow = (ppu->sprite_count > 8);
        }

        // load sprite shifters
        if (ppu->cycles == 340) {
            for (int i = 0; i < ppu->sprite_count; i++) {
                bool flip_vertical = (ppu->secondary_oam[i].attributes >> 7) & 0x1;
                bool flip_horizontal = (ppu->secondary_oam[i].attributes >> 6) & 0x1;

                if (ppu->control.sprite_height == 0) {
                    uint8_t y_offset = flip_vertical ? (7 - (ppu->scanline - ppu->secondary_oam[i].y)) : (ppu->scanline - ppu->secondary_oam[i].y);
                    uint8_t pattern_index = ppu->secondary_oam[i].tile_index;

                    ppu->sprite_shifter_pattern_lo[i] = ppu_read_from_bus(ppu->bus, (ppu->control.sprite_tile_select * 0x1000) + (pattern_index * 0x10) + y_offset);
                    ppu->sprite_shifter_pattern_hi[i] = ppu_read_from_bus(ppu->bus, (ppu->control.sprite_tile_select * 0x1000) + (pattern_index * 0x10) + y_offset + 8);

                    if (flip_horizontal) {
                        ppu->sprite_shifter_pattern_lo[i] = flip_byte(ppu->sprite_shifter_pattern_lo[i]);
                        ppu->sprite_shifter_pattern_hi[i] = flip_byte(ppu->sprite_shifter_pattern_hi[i]);
                    }
                }

                else {
                    uint8_t y_offset = flip_vertical ? (15 - (ppu->scanline - ppu->secondary_oam[i].y)) : (ppu->scanline - ppu->secondary_oam[i].y);
                    uint8_t pattern_index = ppu->secondary_oam[i].tile_index;
                    uint16_t pattern_table_start = (pattern_index & 0x1) * 0x1000;

                    ppu->sprite_shifter_pattern_lo[i] = ppu_read_from_bus(ppu->bus, pattern_table_start + (pattern_index * 0x10) + y_offset);
                    ppu->sprite_shifter_pattern_hi[i] = ppu_read_from_bus(ppu->bus, pattern_table_start + (pattern_index * 0x10) + y_offset + 8);

                    if (flip_horizontal) {
                        ppu->sprite_shifter_pattern_lo[i] = flip_byte(ppu->sprite_shifter_pattern_lo[i]);
                        ppu->sprite_shifter_pattern_hi[i] = flip_byte(ppu->sprite_shifter_pattern_hi[i]);
                    }
                }
            }
        }

        // reset y position for background rendering
        if (ppu->scanline == -1 && ppu->cycles >= 280 && ppu->cycles < 305) {
            // end of vertical blank period so reset y
            transfer_address_y(ppu);
        }
    }

    // rendering
    if ((ppu->scanline >= 0 && ppu->scanline < 240) && (ppu->cycles >= 1 && ppu->cycles < 257)) {
        int scale = SDL_GetWindowSurface(window)->w / 256;
        int x_start = (ppu->cycles - 1) * scale;
        int y_start = ppu->scanline * scale;

        // background rendering
        uint8_t bg_pixel = 0x00;
        uint8_t bg_palette = 0x00;
        uint16_t palette_address = 0x3f00;

        if (ppu->mask.background_enable) {
            uint16_t bit_mux = 0x8000 >> ppu->fine_x;  // select correct bit from the highest 8 bits in the shift register

            uint8_t p0_pixel = (ppu->bg_shifter_pattern_lo & bit_mux) > 0;
            uint8_t p1_pixel = (ppu->bg_shifter_pattern_hi & bit_mux) > 0;

            bg_pixel = (p1_pixel << 1) | p0_pixel;

            uint8_t bg_pal0 = (ppu->bg_shifter_attribute_lo & bit_mux) > 0;
            uint8_t bg_pal1 = (ppu->bg_shifter_attribute_hi & bit_mux) > 0;

            bg_palette = (bg_pal1 << 1) | bg_pal0;

            palette_address += (bg_pixel == 0) ? 0x00 : (bg_palette << 2) + bg_pixel;

            int x = x_start;
            int y = y_start;

            for (int i = 0; i < scale * scale; i++) {
                x = x_start + (i % scale);
                y = y_start + (i / scale);
                set_pixel(window, x, y, SYSTEM_PALETTE[ppu_read_from_bus(ppu->bus, palette_address)]);
            }
            // printf("rendering bg\n");
        }

        // sprite rendering
        uint8_t sprite_pixel = 0x00;
        uint8_t sprite_palette = 0x00;
        uint8_t p0_pixel = 0x00;
        uint8_t p1_pixel = 0x00;
        palette_address = 0x3f10;

        if (ppu->mask.sprite_enable) {
            // reset sprite zero hit flag
            ppu->sprite_zero_rendered = false;
            // render each sprite
            for (int i = 0; i < ppu->sprite_count; i++) {
                // check if the correct x location has been reached
                if (ppu->secondary_oam[i].x == 0) {
                    // get pixel value
                    p0_pixel = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0;
                    p1_pixel = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0;
                    sprite_pixel = (p1_pixel << 1) | p0_pixel;

                    // render only if pixel is opaque
                    if (sprite_pixel != 0) {
                        // check for sprite zero hit
                        if (i == 0) {
                            ppu->sprite_zero_rendered = true;
                        }

                        // check for sprite priority
                        if ((ppu->secondary_oam[i].attributes >> 5) & 0x1) {
                            continue;
                        }

                        // render only if sprite has priority
                        sprite_palette = ppu->secondary_oam[i].attributes & 0x3;
                        palette_address += (sprite_palette << 2) + sprite_pixel;

                        int x = x_start;
                        int y = y_start;

                        for (int i = 0; i < scale * scale; i++) {
                            x = x_start + (i % scale);
                            y = y_start + (i / scale);
                            set_pixel(window, x, y, SYSTEM_PALETTE[ppu_read_from_bus(ppu->bus, palette_address)]);
                        }
                    }
                }
            }
        }

        // sprite 0 hit detection
        if (!ppu->status.sprite_zero_hit && ppu->sprite_zero_on_scanline && ppu->sprite_zero_rendered) {
            // only if sprite and background rendering are enabled
            if (ppu->mask.background_enable && ppu->mask.sprite_enable) {
                // left column mode
                if (!(ppu->mask.background_left_column_enable | ppu->mask.sprite_left_column_enable)) {
                    // correct x location
                    if (ppu->cycles > 8 && ppu->cycles + 7 < 256) {
                        ppu->status.sprite_zero_hit = 1;
                    }
                }

                else {
                    // correct x location
                    if (ppu->cycles + 7 < 256) {
                        ppu->status.sprite_zero_hit = 1;
                    }
                }
            }
        }
    }

    // vblank and nmi
    if (ppu->scanline >= 241 && ppu->scanline < 261) {
        if (ppu->scanline == 241 && ppu->cycles == 1) {
            ppu->status.vblank = 1;
            if (ppu->control.nmi_enable)
                ppu->nmi = true;
        }
    }

    // reset cycles/scanline
    ppu->cycles++;
    if (ppu->cycles >= 341) {
        ppu->cycles = 0;
        ppu->scanline++;

        if (ppu->scanline >= 261) {
            ppu->scanline = -1;
        }
    }
}