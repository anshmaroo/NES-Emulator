#include "2C02.h"

#include "bus.h"

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
        case 0x00:  // CONTROLLER
            ppu->controller.nmi_enable = ((value) >> 7) & 0x1;
            ppu->controller.master_slave_select = ((value) >> 6) & 0x1;
            ppu->controller.sprite_height = ((value) >> 5) & 0x1;
            ppu->controller.background_tile_select = ((value) >> 4) & 0x1;
            ppu->controller.sprite_tile_select = ((value) >> 3) & 0x1;
            ppu->controller.vram_increment_mode = ((value) >> 2) & 0x1;
            ppu->controller.nametable_select = (((value) >> 1) & 0x1) | (value & 0x1);
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
            if(ppu->address_latch == 0) {
                ppu->address = (value << 8) | 0xff00;
                ppu->address_latch = 1;
            }

            else if(ppu->address_latch == 1) {
                ppu->address |= value;
                ppu->address_latch = 0;
            }

            break;

        case 0x07:  // PPUDATA
            ppu->ppudata.data = value;
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
        case 0x00:  // CONTROLLER
            value |= ppu->controller.nmi_enable << 7;
            value |= ppu->controller.master_slave_select << 6;
            value |= ppu->controller.sprite_height << 5;
            value |= ppu->controller.background_tile_select << 4;
            value |= ppu->controller.sprite_tile_select << 3;
            value |= ppu->controller.vram_increment_mode << 2;
            value |= ppu->controller.nametable_select;
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
            ppu->status.vblank = 1;

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
            
            if(ppu->address > 0x3f00)
                value = ppu->data_buffer;

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
    int height = SDL_GetWindowSurface(window)->h;

    // FILE *chr = fopen("pattern_table.txt", "w+");

    uint32_t colors[4] = {0x00000, 0x555555, 0xbbbbbb, 0xffffff};
    int count = 0;
    for (int tile_address = 0; tile_address < 0x2000; tile_address += 16) {
        // printf("CHR ROM tile_address = %04x\n", tile_address);
        for (int byte = tile_address; byte < tile_address + 8; byte++) {
            for (int bit = 0; bit < 8; bit++) {
                uint8_t pixel = ppu_read_from_bus(ppu->bus, byte) >> (7 - (bit % 8)) & 1;
                pixel += (ppu_read_from_bus(ppu->bus, byte + 8) >> (7 - (bit % 8)) & 1) * 2;
                int tile_x = (tile_address / 16) % (width / 8);        // Tile's x position within the array
                int tile_y = (tile_address / (16 * (width / 8))) * 8;  // Tile's y position within the array
                int x = tile_x * 8 + bit;                              // Calculate the x coordinate within the tile
                int y = tile_y + (byte % 8);                           // Calculate the y coordinate within the tile
                if (x > width || x < 0) {
                    printf("X VALUE %d TOO LARGE IN ADDRESS %d!\n", x, byte);
                    // getchar();
                }
                if (y > height || y < 0) {
                    printf("Y VALUE %d TOO LARGE IN ADDRESS %d!\n", y, byte);
                    // getchar();
                }
                set_pixel(window, x, y, colors[pixel]);
                // fprintf(chr, "%d ", pixel);
                count += 1;
            }
        }
        // if(tile_address % 8 == 0 && tile_address % 16 != 0) {
        //     tile_address += 8;
        // }
    }

    SDL_UpdateWindowSurface(window);
}