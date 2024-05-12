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

    state->primary_oam = malloc(0x100);
    state->secondary_oam = malloc(0x20);

    state->sprite_shifter_pattern_lo = malloc(0x8);
    state->sprite_shifter_pattern_lo = malloc(0x8);

    state->bus = NULL;
    state->cycles = 0;
    state->scanline = 0;

    return state;
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

            ppu->tram_address.nametable_x = ppu->control.nametable_select & 0x1;
            ppu->tram_address.nametable_y = ppu->control.nametable_select & 0x2;

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
            write_to_primary_oam(ppu, ppu->oamaddr.address, value);
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

                // printf("TRAM COARSE_Y = %d\n", ppu->tram_address.coarse_y);
                // printf("TRAM FINE_Y = %d\n\n", ppu->tram_address.fine_y);
            }
            break;

        case 0x06:  // PPUADDR
            // getchar();

            if (ppu->w == 0) {
                ppu->tram_address.reg = ((value & 0x3F) << 8) | (ppu->tram_address.reg & 0x00FF);
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
            // if(ppu->address >= 0x3f00 && ppu->address <= 0x3f1f) {
            //     printf("WROTE $%02x TO ADDRESS $%04X\n", value, ppu->address);
            // }

            ppu->vram_address.reg += (ppu->control.vram_increment_mode == 1) ? 32 : 1;
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
            ppu->w = 0;
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
            ppu->data_buffer = ppu_read_from_bus(ppu->bus, ppu->vram_address.reg);

            if (ppu->vram_address.reg >= 0x3f00)
                value = ppu_read_from_bus(ppu->bus, ppu->vram_address.reg);

            ppu->vram_address.reg += (ppu->control.vram_increment_mode == 1) ? 32 : 1;
            ppu->vram_address.reg &= 0x3fff;

            break;

        case 0x4014:  // OAMDMA
            value = ppu->oamdma.address_high_byte;
            break;
    }

    return value;
}

void write_to_primary_oam(State2C02 *ppu, uint8_t address, uint8_t value) {
    ppu->primary_oam[address] = value;
}

uint8_t read_from_primary_oam(State2C02 *ppu, uint8_t address) {
    return ppu->primary_oam[address];
}

void write_to_secondary_oam(State2C02 *ppu, uint8_t address, uint8_t value) {
    ppu->secondary_oam[address] = value;
}

uint8_t read_from_secondary_oam(State2C02 *ppu, uint8_t address) {
    return ppu->secondary_oam[address];
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
 * @brief increases vram x position and switches to next nametable if necessary
 *
 * @param ppu
 */
void increment_scroll_x(State2C02 *ppu) {
    if (ppu->mask.background_enable || ppu->mask.sprite_enable) {
        if (ppu->vram_address.coarse_x == 31) {
            ppu->vram_address.coarse_x = 0;
            ppu->vram_address.nametable_x = ~ppu->vram_address.nametable_x;
        } else {
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

                ppu->vram_address.nametable_y = ~ppu->vram_address.nametable_y;
            } else if (ppu->vram_address.coarse_y == 31) {
                ppu->vram_address.coarse_y = 0;
            } else {
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
    ppu->bg_shifter_pattern_lo <<= 1;
    ppu->bg_shifter_pattern_hi <<= 1;
    ppu->bg_shifter_attribute_lo <<= 1;
    ppu->bg_shifter_attribute_hi <<= 1;
}

/**
 * @brief shifts sprite "shift registers" by one bit
 *
 * @param ppu
 */
void update_sprite_shifters(State2C02 *ppu) {
    for (int i = 0; i < ppu->sprite_count; i++) {
        uint8_t x = read_from_secondary_oam(ppu, (i * 4) + 3);
        if (x > 0) {
            write_to_secondary_oam(ppu, (i * 4) + 3, x - 1);
        }

        else {
            ppu->sprite_shifter_pattern_lo[i] <<= 1;
            ppu->sprite_shifter_pattern_hi[i] <<= 1;
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
        // printf("WRITING TO OAM, CLOCK CYCLE = %d\n", bus->oam_clock);
        if (ppu->oamdma_clock % 2 == 0) {
            uint8_t value = cpu_read_from_bus(ppu->bus, ppu->oamdma.address_high_byte << 8 | (ppu->oamdma_clock / 2));
            write_to_primary_oam(ppu, ppu->oamaddr.address, value);
            // if (ppu->oamdma_clock % 8 == 0)
            //     printf("\n");
            // printf("%02x ", value);
            ppu->oamaddr.address++;
        }

        if (ppu->oamdma_clock == 512) {
            ppu->oamdma_clock = 0;
            ppu->oamdma_write = false;
        }

        else {
            ppu->oamdma_clock++;
        }
    }

    if (ppu->scanline >= -1 && ppu->scanline < 240) {
        // odd cycle switch
        if (ppu->scanline == 0 && ppu->cycles == 0) {
            ppu->cycles = 1;
        }

        // vblank
        if (ppu->scanline == -1 && ppu->cycles == 1) {
            ppu->status.vblank = 0;
            ppu->status.sprite_overflow = 0;
            ppu->sprite_count = 0;
            for (int i = 0; i < 8; i++) {
                ppu->sprite_shifter_pattern_lo[i] = 0;
                ppu->sprite_shifter_pattern_lo[i] = 0;
            }
        }

        // initialize secondary OAM to $FF
        if (ppu->cycles >= 1 && ppu->cycles < 65) {
            write_to_secondary_oam(ppu, ppu->cycles - 1, 0xff);
        }

        // iterate through primary OAM and load secondary OAM
        if (ppu->cycles >= 65 && ppu->cycles < 257) {
            // TODO - LOAD SECONDARY OAM
            uint8_t oam_value = 0;
            if (ppu->cycles % 2 == 1 && !ppu->status.sprite_overflow) {
                oam_value = read_from_primary_oam(ppu, ppu->primary_oam_address);

            }

            else if (ppu->cycles % 2 == 0) {
                if (ppu->sprite_count < 9) {
                    if (ppu->primary_oam_address % 4 == 0) {
                        // check y-value
                        uint8_t y_value = (oam_value - 1);
                        if (ppu->scanline >= y_value && ppu->scanline < y_value + (ppu->control.sprite_height * 8) + 8) {
                            write_to_secondary_oam(ppu, ppu->secondary_oam_address, oam_value);
                            ppu->sprite_count++;
                            ppu->primary_oam_address++;
                            ppu->secondary_oam_address++;
                        } else {
                            ppu->primary_oam_address += 4;
                        }
                    }

                    else {
                        write_to_secondary_oam(ppu, ppu->secondary_oam_address, oam_value);
                        ppu->secondary_oam_address++;
                    }
                } else {
                    ppu->status.sprite_overflow = 1;
                }
            }
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

        // load sprite shifters
        if (ppu->cycles == 320) {
            for (int i = 0; i < ppu->sprite_count; i++) {
                if (ppu->control.sprite_height == 0) {
                    uint8_t y_offset = ppu->scanline - (read_from_secondary_oam(ppu, (i * 4)) - 1);
                    uint8_t pattern_index = read_from_secondary_oam(ppu, (i * 4) + 1);

                    ppu->sprite_shifter_pattern_lo[i] = ppu_read_from_bus(ppu->bus, (ppu->control.background_tile_select * 0x1000) + (pattern_index * 0x10) + y_offset);
                    ppu->sprite_shifter_pattern_hi[i] = ppu_read_from_bus(ppu->bus, (ppu->control.background_tile_select * 0x1000) + (pattern_index * 0x10) + y_offset + 8);
                }

                else {
                    uint8_t y_offset = ppu->scanline - (read_from_secondary_oam(ppu, (i * 4)) - 1);
                    uint8_t pattern_index = read_from_secondary_oam(ppu, (i * 4) + 1);
                    uint16_t pattern_table_start = (pattern_index & 0x1) * 0x1000;

                    ppu->sprite_shifter_pattern_lo[i] = ppu_read_from_bus(ppu->bus, pattern_table_start + (pattern_index * 0x10) + y_offset);
                    ppu->sprite_shifter_pattern_hi[i] = ppu_read_from_bus(ppu->bus, pattern_table_start + (pattern_index * 0x10) + y_offset + 8);
                }
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

        if (ppu->scanline == -1 && ppu->cycles >= 280 && ppu->cycles < 305) {
            // end of vertical blank period so reset y
            transfer_address_y(ppu);
        }
    }

    if (ppu->scanline >= 241 && ppu->scanline < 261) {
        if (ppu->scanline == 241 && ppu->cycles == 1) {
            ppu->status.vblank = 1;
            if (ppu->control.nmi_enable)
                ppu->nmi = true;
        }
    }

    // rendering
    if ((ppu->scanline >= 0 && ppu->scanline <= 239) && (ppu->cycles >= 0 && ppu->cycles <= 255)) {
        int scale = SDL_GetWindowSurface(window)->w / 256;
        int x_start = (ppu->cycles - 1) * scale;
        int y_start = ppu->scanline * scale;

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

            palette_address += (bg_palette << 2) + bg_pixel;

            int x = x_start;
            int y = y_start;

            for (int i = 0; i < scale * scale; i++) {
                x = x_start + (i % scale);
                y = y_start + (i / scale);
                set_pixel(window, x, y, SYSTEM_PALETTE[ppu_read_from_bus(ppu->bus, palette_address)]);
            }
        }

        uint8_t sprite_pixel = 0x00;
        uint8_t sprite_palette = 0x00;
        palette_address = 0x3f10;

        if (ppu->mask.sprite_enable) {
            for (int i = 0; i < ppu->sprite_count; i++) {
                if (read_from_secondary_oam(ppu, (i * 4) + 3) == 0) {
                    printf("entered sprite rendering\n");
                    uint8_t p0_pixel = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0;
                    uint8_t p1_pixel = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0;
                    
                    sprite_pixel = (p1_pixel << 1) | p0_pixel;
                    printf("sprite pixel = %d\n", sprite_pixel);
                    sprite_palette = (read_from_secondary_oam(ppu, (i * 4) + 2) & 0x3);
                    palette_address += (sprite_palette * 4) + sprite_pixel;

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

    ppu->cycles++;
    if (ppu->cycles >= 341) {
        ppu->cycles = 0;
        ppu->scanline++;

        if (ppu->scanline >= 261) {
            ppu->scanline = -1;
        }
    }
}