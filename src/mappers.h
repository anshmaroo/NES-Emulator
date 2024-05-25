#include "bus.h"

/**
 * @brief select and apply mapper
 * 
 * @param bus 
 * @param filename 
 */
void mapper(Bus *bus, char filename[]);

/**
 * @brief load an NROM game's program rom and CHR-ROM
 *
 * @param cpu
 * @param filename
 */
void nrom(Bus *bus, char filename[]);

/**
 * @brief load an UXROM game's program rom and CHR-ROM
 * 
 * @param bus 
 * @param filename 
 */
void uxrom(Bus *bus, char filename[]);