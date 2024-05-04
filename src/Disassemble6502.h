#include <stdio.h>
#include <stdlib.h>

/**
 * @brief decode 6502 operation from bytes
 * 
 * @param codebuffer 
 * @param pc 
 * @return int 
 */
int Disassemble6502Op(uint8_t *codebuffer, int pc);