
#include "Disassemble6502.h"

/**
 * @brief decode 6502 operation from bytes
 * 
 * @param codebuffer 
 * @param pc 
 * @return int 
 */
char *Disassemble6502Op(uint8_t *codebuffer, int pc) {
    uint8_t *opcodes = &codebuffer[0];
    char *output = malloc(100 * sizeof(char));
    printf("%04x %02x ", pc, opcodes[0]); // print the program opbyteser addressl

    switch (opcodes[0]) 
    {
        // dissasemble hex code into operations in assembly
        case 0x00: sprintf(output, "BRK"); break;
        case 0x01: sprintf(output, "ORA ($%02x, X)", opcodes[1]); break;
        case 0x05: sprintf(output, "ORA $%02x", opcodes[1]); break;
        case 0x06: sprintf(output, "ASL $%02x", opcodes[1]); break;
        case 0x08: sprintf(output, "PHP"); break;
        case 0x09: sprintf(output, "ORA #$%02x", opcodes[1]); break;
        case 0x0a: sprintf(output, "ASL A"); break;
        case 0x0d: sprintf(output, "ORA $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x0e: sprintf(output, "ASL $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x10: sprintf(output, "BPL $%02x", opcodes[1]); break;
        case 0x11: sprintf(output, "ORA ($%02x, Y)", opcodes[1]); break;
        case 0x15: sprintf(output, "ORA $%02x X", opcodes[1]); break;
        case 0x16: sprintf(output, "ASL $%02x X", opcodes[1]); break;
        case 0x18: sprintf(output, "CLC"); break;
        case 0x19: sprintf(output, "ORA $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0x1d: sprintf(output, "ORA $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x1e: sprintf(output, "ASL $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x20: sprintf(output, "JSR $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x21: sprintf(output, "AND ($%02x, X)", opcodes[1]); break;
        case 0x24: sprintf(output, "BIT $%02x", opcodes[1]); break;
        case 0x25: sprintf(output, "AND $%02x", opcodes[1]); break;
        case 0x26: sprintf(output, "ROL $%02x", opcodes[1]); break;
        case 0x28: sprintf(output, "PLP"); break;
        case 0x29: sprintf(output, "AND #$%02x", opcodes[1]); break;
        case 0x2a: sprintf(output, "ROL A"); break;
        case 0x2c: sprintf(output, "BIT $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x2d: sprintf(output, "AND $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x2e: sprintf(output, "ROL $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x30: sprintf(output, "BMI $%02x", opcodes[1]); break;
        case 0x31: sprintf(output, "AND ($%02x, Y)", opcodes[1]); break;
        case 0x35: sprintf(output, "AND $%02x X", opcodes[1]); break;
        case 0x36: sprintf(output, "ROL $%02x X", opcodes[1]); break;
        case 0x38: sprintf(output, "SEC"); break;
        case 0x39: sprintf(output, "AND $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0x3d: sprintf(output, "AND $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x3e: sprintf(output, "ROL $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x40: sprintf(output, "RTI"); break;
        case 0x41: sprintf(output, "EOR ($%02x, X)", opcodes[1]); break;
        case 0x45: sprintf(output, "EOR $%02x", opcodes[1]); break;
        case 0x46: sprintf(output, "LSR $%02x", opcodes[1]); break;
        case 0x48: sprintf(output, "PHA"); break;
        case 0x49: sprintf(output, "EOR #$%02x", opcodes[1]); break;
        case 0x4a: sprintf(output, "LSR A"); break;
        case 0x4c: sprintf(output, "JMP $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x4d: sprintf(output, "EOR $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x4e: sprintf(output, "LSR $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x50: sprintf(output, "BVC $%02x", opcodes[1]); break;
        case 0x51: sprintf(output, "EOR ($%02x, Y)", opcodes[1]); break;
        case 0x55: sprintf(output, "EOR $%02x X", opcodes[1]); break;
        case 0x56: sprintf(output, "LSR $%02x X", opcodes[1]); break;
        case 0x58: sprintf(output, "CLI"); break;
        case 0x59: sprintf(output, "EOR $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0x5d: sprintf(output, "EOR $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x5e: sprintf(output, "LSR $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x60: sprintf(output, "RTS"); break;
        case 0x61: sprintf(output, "ADC ($%02x, X)", opcodes[1]); break;
        case 0x65: sprintf(output, "ADC $%02x", opcodes[1]); break;
        case 0x66: sprintf(output, "ROR $%02x", opcodes[1]); break;
        case 0x68: sprintf(output, "PLA"); break;
        case 0x69: sprintf(output, "ADC #$%02x", opcodes[1]); break;
        case 0x6a: sprintf(output, "ROR A"); break;
        case 0x6c: sprintf(output, "JMP ($%02x%02x)", opcodes[2], opcodes[1]); break;
        case 0x6d: sprintf(output, "ADC $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x6e: sprintf(output, "ROR $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x70: sprintf(output, "BVS $%02x", opcodes[1]); break;
        case 0x71: sprintf(output, "ADC ($%02x, Y)", opcodes[1]); break;
        case 0x75: sprintf(output, "ADC $%02x X", opcodes[1]); break;
        case 0x76: sprintf(output, "ROR $%02x X", opcodes[1]); break;
        case 0x78: sprintf(output, "SEI"); break;
        case 0x79: sprintf(output, "ADC $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0x7d: sprintf(output, "ADC $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0x7e: sprintf(output, "ROR $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x81: sprintf(output, "STA ($%02x, X)", opcodes[1]); break;
        case 0x84: sprintf(output, "STY $%02x", opcodes[1]); break;
        case 0x85: sprintf(output, "STA $%02x", opcodes[1]); break;
        case 0x86: sprintf(output, "STX $%02x", opcodes[1]); break;
        case 0x88: sprintf(output, "DEY"); break;
        case 0x8a: sprintf(output, "TXA"); break;
        case 0x8c: sprintf(output, "STY $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x8d: sprintf(output, "STA $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x8e: sprintf(output, "STX $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0x90: sprintf(output, "BCC $%02x", opcodes[1]); break;
        case 0x91: sprintf(output, "STA ($%02x, Y)", opcodes[1]); break;
        case 0x94: sprintf(output, "STY $%02x X", opcodes[1]); break;
        case 0x95: sprintf(output, "STA $%02x X", opcodes[1]); break;
        case 0x96: sprintf(output, "STX $%02x X", opcodes[1]); break;
        case 0x98: sprintf(output, "TYA"); break;
        case 0x99: sprintf(output, "STA $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0x9a: sprintf(output, "TXS"); break;
        case 0x9d: sprintf(output, "STA $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0xa0: sprintf(output, "LDY #$%02x", opcodes[1]); break;
        case 0xa1: sprintf(output, "LDA ($%02x, X)", opcodes[1]); break;
        case 0xa2: sprintf(output, "LDX #$%02x", opcodes[1]); break;
        case 0xa4: sprintf(output, "LDY $%02x", opcodes[1]); break;
        case 0xa5: sprintf(output, "LDA $%02x", opcodes[1]); break;
        case 0xa6: sprintf(output, "LDX $%02x", opcodes[1]); break;
        case 0xa8: sprintf(output, "TAY"); break;
        case 0xa9: sprintf(output, "LDA #$%02x", opcodes[1]); break;
        case 0xaa: sprintf(output, "TAX"); break;
        case 0xac: sprintf(output, "LDY $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xad: sprintf(output, "LDA $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xae: sprintf(output, "LDX $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xb0: sprintf(output, "BCS $%02x", opcodes[1]); break;
        case 0xb1: sprintf(output, "LDA ($%02x, Y)", opcodes[1]); break;
        case 0xb4: sprintf(output, "LDY $%02x X", opcodes[1]); break;
        case 0xb5: sprintf(output, "LDA $%02x X", opcodes[1]); break;
        case 0xb6: sprintf(output, "LDX $%02x X", opcodes[1]); break;
        case 0xb8: sprintf(output, "CLV"); break;
        case 0xb9: sprintf(output, "LDA $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0xba: sprintf(output, "TSX"); break;
        case 0xbc: sprintf(output, "LDY $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0xbd: sprintf(output, "LDA $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0xbe: sprintf(output, "LDX $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0xc0: sprintf(output, "CPY #$%02x", opcodes[1]); break;
        case 0xc1: sprintf(output, "CMP ($%02x, X)", opcodes[1]); break;
        case 0xc4: sprintf(output, "CPY $%02x", opcodes[1]); break;
        case 0xc5: sprintf(output, "CMP $%02x", opcodes[1]); break;
        case 0xc6: sprintf(output, "DEC $%02x", opcodes[1]); break;
        case 0xc8: sprintf(output, "INY"); break;
        case 0xc9: sprintf(output, "CMP #$%02x", opcodes[1]); break;
        case 0xca: sprintf(output, "DEX"); break;
        case 0xcc: sprintf(output, "CPY $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xcd: sprintf(output, "CMP $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xce: sprintf(output, "DEC $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xd0: sprintf(output, "BNE $%02x", opcodes[1]); break;
        case 0xd1: sprintf(output, "CMP ($%02x, Y)", opcodes[1]); break;
        case 0xd5: sprintf(output, "CMP $%02x X", opcodes[1]); break;
        case 0xd6: sprintf(output, "DEC $%02x X", opcodes[1]); break;
        case 0xd8: sprintf(output, "CLD"); break;
        case 0xd9: sprintf(output, "CMP $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0xdd: sprintf(output, "CMP $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0xde: sprintf(output, "DEC $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0xe0: sprintf(output, "CPX #$%02x", opcodes[1]); break;
        case 0xe1: sprintf(output, "SBC ($%02x, X)", opcodes[1]); break;
        case 0xe4: sprintf(output, "CPX $%02x", opcodes[1]); break;
        case 0xe5: sprintf(output, "SBC $%02x", opcodes[1]); break;
        case 0xe6: sprintf(output, "INC $%02x", opcodes[1]); break;
        case 0xe8: sprintf(output, "INX"); break;
        case 0xe9: sprintf(output, "SBC #$%02x", opcodes[1]); break;
        case 0xea: sprintf(output, "NOP"); break;
        case 0xec: sprintf(output, "CPX $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xed: sprintf(output, "SBC $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xee: sprintf(output, "INC $%02x%02x", opcodes[2], opcodes[1]); break;
        case 0xf0: sprintf(output, "BEQ $%02x", opcodes[1]); break;
        case 0xf1: sprintf(output, "SBC ($%02x, Y)", opcodes[1]); break;
        case 0xf5: sprintf(output, "SBC $%02x X", opcodes[1]); break;
        case 0xf6: sprintf(output, "INC $%02x X", opcodes[1]); break;
        case 0xf8: sprintf(output, "SED"); break;
        case 0xf9: sprintf(output, "SBC $%02x%02x Y", opcodes[2], opcodes[1]); break;
        case 0xfd: sprintf(output, "SBC $%02x%02x X", opcodes[2], opcodes[1]); break;
        case 0xfe: sprintf(output, "INC $%02x%02x X", opcodes[2], opcodes[1]); break;
        default: sprintf(output, "INVALID OPERATION: %02x", opcodes[0]); break;

    }
    
    return output;
}



