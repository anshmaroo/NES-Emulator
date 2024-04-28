#include "Disassemble6502.h"
#include <stdint.h>

/**
 * @brief decode 6502 operation from bytes
 * 
 * @param codebuffer 
 * @param pc 
 * @return int 
 */
int Disassemble6502Op(uint8_t *codebuffer, int pc) {
    uint8_t *opcodes = &codebuffer[0];
    int opbytes = 1; // set # of bytes for operation to 1 by default
    printf("%04x %02x ", pc, opcodes[0]); // print the program opbyteser addressl

    switch (opcodes[0]) 
    {
        // dissasemble hex code into operations in assembly
        case 0x00: printf("BRK"); break;
        case 0x01: printf("ORA ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0x05: printf("ORA $%02x", opcodes[1]); opbytes = 2; break;
        case 0x06: printf("ASL $%02x", opcodes[1]); opbytes = 2; break;
        case 0x08: printf("PHP"); break;
        case 0x09: printf("ORA #$%02x", opcodes[1]); opbytes = 2; break;
        case 0x0a: printf("ASL A"); break;
        case 0x0d: printf("ORA $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x0e: printf("ASL $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x10: printf("BPL $%02x", opcodes[1]); break;
        case 0x11: printf("ORA ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0x15: printf("ORA $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x16: printf("ASL $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x18: printf("CLC"); break;
        case 0x19: printf("ORA $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x1d: printf("ORA $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x1e: printf("ASL $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x20: printf("JSR $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x21: printf("AND ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0x24: printf("BIT $%02x", opcodes[1]); opbytes = 2; break;
        case 0x25: printf("AND $%02x", opcodes[1]); opbytes = 2; break;
        case 0x26: printf("ROL $%02x", opcodes[1]); opbytes = 2; break;
        case 0x28: printf("PLP"); break;
        case 0x29: printf("AND #$%02x", opcodes[1]); opbytes = 2; break;
        case 0x2a: printf("ROL A"); break;
        case 0x2c: printf("BIT $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x2d: printf("AND $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x2e: printf("ROL $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x30: printf("BMI $%02x", opcodes[1]); break;
        case 0x31: printf("AND ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0x35: printf("AND $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x36: printf("ROL $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x38: printf("SEC"); break;
        case 0x39: printf("AND $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x3d: printf("AND $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x3e: printf("ROL $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x40: printf("RTI"); break;
        case 0x41: printf("EOR ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0x45: printf("EOR $%02x", opcodes[1]); opbytes = 2; break;
        case 0x46: printf("LSR $%02x", opcodes[1]); opbytes = 2; break;
        case 0x48: printf("PHA"); break;
        case 0x49: printf("EOR #$%02x", opcodes[1]); opbytes = 2; break;
        case 0x4a: printf("LSR A"); break;
        case 0x4c: printf("JMP $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x4d: printf("EOR $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x4e: printf("LSR $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x50: printf("BVC $%02x", opcodes[1]); break;
        case 0x51: printf("EOR ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0x55: printf("EOR $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x56: printf("LSR $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x58: printf("CLI"); break;
        case 0x59: printf("EOR $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x5d: printf("EOR $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x5e: printf("LSR $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x60: printf("RTS"); break;
        case 0x61: printf("ADC ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0x65: printf("ADC $%02x", opcodes[1]); opbytes = 2; break;
        case 0x66: printf("ROR $%02x", opcodes[1]); opbytes = 2; break;
        case 0x68: printf("PLA"); break;
        case 0x69: printf("ADC #$%02x", opcodes[1]); opbytes = 2; break;
        case 0x6a: printf("ROR A"); break;
        case 0x6c: printf("JMP ($%02x%02x)", opcodes[2], opcodes[1]); opbytes = 2; break;
        case 0x6d: printf("ADC $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x6e: printf("ROR $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x70: printf("BVS $%02x", opcodes[1]); break;
        case 0x71: printf("ADC ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0x75: printf("ADC $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x76: printf("ROR $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x78: printf("SEI"); break;
        case 0x79: printf("ADC $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x7d: printf("ADC $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x7e: printf("ROR $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x81: printf("STA ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0x84: printf("STY $%02x", opcodes[1]); opbytes = 2; break;
        case 0x85: printf("STA $%02x", opcodes[1]); opbytes = 2; break;
        case 0x86: printf("STX $%02x", opcodes[1]); opbytes = 2; break;
        case 0x88: printf("DEY"); break;
        case 0x8a: printf("TXA"); break;
        case 0x8c: printf("STY $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x8d: printf("STA $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x8e: printf("STX $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x90: printf("BCC $%02x", opcodes[1]); break;
        case 0x91: printf("STA ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0x94: printf("STY $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x95: printf("STA $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x96: printf("STX $%02x X", opcodes[1]); opbytes = 2; break;
        case 0x98: printf("TYA"); break;
        case 0x99: printf("STA $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0x9a: printf("TXS"); break;
        case 0x9d: printf("STA $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xa0: printf("LDY #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xa1: printf("LDA ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0xa2: printf("LDX #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xa4: printf("LDY $%02x", opcodes[1]); opbytes = 2; break;
        case 0xa5: printf("LDA $%02x", opcodes[1]); opbytes = 2; break;
        case 0xa6: printf("LDX $%02x", opcodes[1]); opbytes = 2; break;
        case 0xa8: printf("TAY"); break;
        case 0xa9: printf("LDA #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xaa: printf("TAX"); break;
        case 0xac: printf("LDY $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xad: printf("LDA $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xae: printf("LDX $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xb0: printf("BCS $%02x", opcodes[1]); break;
        case 0xb1: printf("LDA ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0xb4: printf("LDY $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xb5: printf("LDA $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xb6: printf("LDX $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xb8: printf("CLV"); break;
        case 0xb9: printf("LDA $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xba: printf("TSX"); break;
        case 0xbc: printf("LDY $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xbd: printf("LDA $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xbe: printf("LDX $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xc0: printf("CPY #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xc1: printf("CMP ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0xc4: printf("CPY $%02x", opcodes[1]); opbytes = 2; break;
        case 0xc5: printf("CMP $%02x", opcodes[1]); opbytes = 2; break;
        case 0xc6: printf("DEC $%02x", opcodes[1]); opbytes = 2; break;
        case 0xc8: printf("INY"); break;
        case 0xc9: printf("CMP #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xca: printf("DEX"); break;
        case 0xcc: printf("CPY $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xcd: printf("CMP $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xce: printf("DEC $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xd0: printf("BNE $%02x", opcodes[1]); break;
        case 0xd1: printf("CMP ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0xd5: printf("CMP $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xd6: printf("DEC $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xd8: printf("CLD"); break;
        case 0xd9: printf("CMP $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xdd: printf("CMP $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xde: printf("DEC $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xe0: printf("CPX #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xe1: printf("SBC ($%02x, X)", opcodes[1]); opbytes = 2; break;
        case 0xe4: printf("CPX $%02x", opcodes[1]); opbytes = 2; break;
        case 0xe5: printf("SBC $%02x", opcodes[1]); opbytes = 2; break;
        case 0xe6: printf("INC $%02x", opcodes[1]); opbytes = 2; break;
        case 0xe8: printf("INX"); break;
        case 0xe9: printf("SBC #$%02x", opcodes[1]); opbytes = 2; break;
        case 0xea: printf("NOP"); break;
        case 0xec: printf("CPX $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xed: printf("SBC $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xee: printf("INC $%02x%02x", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xf0: printf("BEQ $%02x", opcodes[1]); break;
        case 0xf1: printf("SBC ($%02x, Y)", opcodes[1]); opbytes = 2; break;
        case 0xf5: printf("SBC $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xf6: printf("INC $%02x X", opcodes[1]); opbytes = 2; break;
        case 0xf8: printf("SED"); break;
        case 0xf9: printf("SBC $%02x%02x Y", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xfd: printf("SBC $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        case 0xfe: printf("INC $%02x%02x X", opcodes[2], opcodes[1]); opbytes = 3; break;
        default: printf("INVALID OPERATION: %02x", opcodes[0]); break;

    }
    printf("\n");
    return opbytes;
}



