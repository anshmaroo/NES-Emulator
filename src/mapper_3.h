#include "mapper.h"
#include "bus.h"

/**
 * @brief load initial CPU and PPU address space
 * 
 * @param bus 
 * @param filename 
 */
void cnrom_initialize(MapperInterface *mapper, Bus *bus);


/**
 * @brief switch chr bank
 * 
 * @param mapper 
 * @param bus 
 * @param current_address 
 * @param chr_bank 
 */
void cnrom_switch_chr_banks(struct MapperInterface *mapper, Bus *bus, uint16_t current_address, uint8_t bank_number);