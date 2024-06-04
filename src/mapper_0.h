#include "mapper.h"
#include "bus.h"

/**
 * @brief load initial CPU and PPU address space
 * 
 * @param bus 
 * @param filename 
 */
void nrom_initialize(MapperInterface *mapper, Bus *bus);


/**
 * @brief do nothing
 * 
 * @param mapper 
 * @param bus 
 * @param current_address 
 * @param prg_bank 
 */
void nrom_switch_prg_banks(struct MapperInterface *mapper, Bus *bus, uint16_t current_address, uint8_t prg_bank);