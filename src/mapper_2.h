#include "mapper.h"
#include "bus.h"

/**
 * @brief load initial CPU and PPU address space
 * 
 * @param bus 
 * @param filename 
 */
void uxrom_initialize(MapperInterface *mapper, Bus *bus);

/**
 * @brief switch prg ROM bank
 * 
 * @param bus 
 * @param filename 
 */
void uxrom_switch_prg_banks(MapperInterface *mapper, Bus *bus, uint16_t current_address, uint8_t bank_number);
