#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define DEBUG true


#ifndef __6502_H__c
#define __6502_H__
static const uint8_t OPCODES_CYCLES[256] = {
    // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,  // 0
    2, 5, 0, 8, 4, 3, 5, 6, 2, 5, 2, 7, 5, 4, 7, 7,  // 1
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,  // 2
    6, 5, 0, 8, 4, 4, 6, 6, 2, 5, 2, 7, 5, 4, 7, 7,  // 3
    6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,  // 4
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 5, 7, 5, 4, 7, 7,  // 5
    6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,  // 6
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 5, 4, 7, 7,  // 7
    0, 6, 2, 6, 3, 3, 3, 3, 4, 2, 2, 2, 4, 4, 4, 4,  // 8
    2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,  // 9
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,  // A
    2, 5, 0, 6, 4, 4, 4, 4, 2, 4, 2, 5, 4, 4, 4, 5,  // B
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,  // C
    2, 2, 0, 8, 4, 3, 5, 6, 2, 4, 2, 7, 5, 4, 7, 7,  // D
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,  // E
    2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 5, 4, 7, 7   // F

};

static const uint8_t OPCODES_BYTES[256] = {
 // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    0, 2, 0, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  // 0
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 2, 3, 3, 3, 3, 3,  // 1
    0, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  // 2
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,  // 3
    0, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 0, 3, 3, 3,  // 4
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,  // 5
    1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 0, 3, 3, 3,  // 6
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,  // 7
    2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  // 8
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,  // 9
    2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  // A
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,  // B
    2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  // C
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3,  // D
    2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 3, 3, 3, 3,  // E
    2, 2, 1, 2, 2, 2, 2, 2, 1, 3, 1, 3, 3, 3, 3, 3   // F

};


typedef struct StatusRegister {
    bool n;  // bit 7, 1 = negative
    bool v;  // bit 6, 1 = overflow
    bool b;  // bit 4, 1 = BRK, 0 = IRQB
    bool d;  // bit 3, 1 = decimal mode
    bool i;  // bit 2, 1 = IRQB disable
    bool z;  // bit 1, 1 = zero
    bool c;  // bit 0, 1 = carry

} StatusRegister;

typedef struct State6502 {
    uint8_t a;
    uint8_t x;
    uint8_t y;

    StatusRegister sr;

    uint8_t sp;
    uint16_t pc;

    uint8_t int_enable;
    uint8_t halted;
    uint16_t cycles;

    struct Bus *bus;
    
    // uint8_t *memory;

} State6502;
#endif


/**
 * @brief initializes a State6502 object
 *
 * @return State6502*
 */
State6502 *Init6502(void);

/************************ STATUS REGISTER FUNCTIONS ************************/
/**
 * @brief Get all the status register flags in one byte
 *
 * @param cpu
 * @return uint8_t
 */
static inline uint8_t get_status_register(State6502 *cpu);

/**
 * @brief set the status register flags from one byte
 *
 * @param cpu
 * @param byte
 */
static inline void set_status_register(State6502 *cpu, uint8_t byte);

/**
 * @brief updates the zero and negative flags based on the result of the
 * previous operation
 *
 * @param cpu
 * @param value the result of an operation to updates flags based on
 */
static inline void update_zn(State6502 *cpu, uint8_t value);

/**
 * @brief set negative and overflow flags to bits 7 and 6 of the value provided.
 * sets the zero flag by ANDing with A register
 *
 * @param cpu
 * @param value value provided
 */
static inline void bit(State6502 *cpu, uint8_t value);

/************************ LOGICAL FUNCTIONS ************************/
/**
 * @brief logical OR operation with the accumulator
 *
 * @param cpu
 * @param value value to OR with a
 */
static inline void or_a(State6502 *cpu, uint8_t value);

/**
 * @brief logical AND operation with the accumulator
 *
 * @param cpu
 * @param value value to AND with a
 */
static inline void and_a(State6502 *cpu, uint8_t value);

/**
 * @brief exclusive or with the accumulator
 *
 * @param cpu
 * @param value value to XOR with accumulator
 */
static inline void xor_a(State6502 *cpu, uint8_t value);

/**
 * @brief rotate value in memory left by one bit
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void asl_memory(State6502 *cpu, uint16_t address);

/**
 * @brief rotate value in accumulator left by one bit
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void asl_a(State6502 *cpu);

/**
 * @brief shifts a value in memory one bit to the left
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void rol_memory(State6502 *cpu, uint16_t address);

/**
 * @brief shifts the value in the accumualtor one bit to the left
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void rol_a(State6502 *cpu);

/**
 * @brief shifts the value one bit to the right
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void ror_memory(State6502 *cpu, uint16_t address);

/**
 * @brief shifts the value in the accumulator one bit to the right
 *
 * @param cpu
 */
static inline void ror_a(State6502 *cpu);

/**
 * @brief shift value in memory right by one bit
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void lsr_memory(State6502 *cpu, uint16_t address);
/**
 * @brief shift value in accumulator right by one bit
 *
 * @param cpu
 */
static inline void lsr_a(State6502 *cpu);


/**
 * @brief compares two values and updates flags based on the result
 *
 * @param cpu
 * @param register_value  value of the register to compare to
 * @param value value that is to be compared with
 */
static inline void cmp(State6502 *cpu, uint8_t register_value, uint8_t value);

/************************ LOAD/STORE/MOVE FUNCTIONS ************************/
/**
 * @brief pushes an 8-bit value to the stack
 * Decreases the stack pointer by one byte, and stores an 8-bit value at the new
 * location of the stack
 * @param cpu
 * @param value the value to be pushed to the stack
 */
static inline void push(State6502 *cpu, uint8_t value);

/**
 * @brief pops a value that was stored on the stack
 * Gets the value stored on the stack and increases the stack pointer by one
 * byte, resetting its original position.
 * @param cpu
 */
static inline uint8_t pop(State6502 *cpu);

/**
 * @brief pushes a 16-bit value to the stack
 * Decreases the stack pointer by two bytes, and stores a 16-bit value at the
 * new location of the stack
 * @param cpu the State6502 object
 * @param value the value to be pushed to the stack
 */
static inline void push_16(State6502 *cpu, uint16_t value);

/**
 * @brief pops a 16-bit value that was stored on the stack
 * Gets the value stored on the stack and increases the stack pointer by two
 * bytes, resetting its original position.
 * @param cpu the State6502 object
 */
static inline uint16_t pop_16(State6502 *cpu);

/**
 * @brief stores accumulator in memory
 *
 * @param cpu
 * @param address address to store accumulator at
 */
static inline void sta(State6502 *cpu, uint16_t address);


/**
 * @brief stores y-register in memory
 *
 * @param cpu
 * @param address address to store accumulator at
 */
static inline void sty(State6502 *cpu, uint16_t address);

/**
 * @brief stores x-register in memory
 *
 * @param cpu
 * @param address address to store accumulator at
 */
static inline void stx(State6502 *cpu, uint16_t address);

/**
 * @brief transfers x-register to accumulator
 *
 * @param cpu
 */
static inline void txa(State6502 *cpu);

/**
 * @brief transfers accumulator to x-register
 *
 * @param cpu
 */
static inline void tax(State6502 *cpu);

/**
 * @brief transfers y-register to accumulator
 *
 * @param cpu
 */
static inline void tya(State6502 *cpu);

/**
 * @brief transfers accumulator to y-register
 * @param cpu
 */
static inline void tay(State6502 *cpu);

/**
 * @brief transfers stack pointer to x-register
 * @param cpu
 */
static inline void tsx(State6502 *cpu);

/**
 * @brief transfers x-register to stack pointer
 * @param cpu
 */
static inline void txs(State6502 *cpu);

/**
 * @brief loads y-register with a value
 *
 * @param cpu
 * @param value value to load y-register with
 */
static inline void ldy(State6502 *cpu, uint8_t value);

/**
 * @brief loads x-register with a value
 *
 * @param cpu
 * @param value value to load x-register with
 */
static inline void ldx(State6502 *cpu, uint8_t value);

/**
 * @brief loads accumulator with a value
 *
 * @param cpu
 * @param value value to load accumulator with
 */
static inline void lda(State6502 *cpu, uint8_t value);

/************************ I/O FUNCTIONS ************************/

/**
 * @brief set pc to reset vector
 *
 * @param cpu
 */
void reset(State6502 *cpu);

/**
 * @brief interrupt from fffe and ffff
 *
 * @param cpu
 */
static inline void irq(State6502 *cpu);

/**
 * @brief interrupt an interrupt
 *
 * @param cpu
 */
void nmi(State6502 *cpu);

/************************ BRANCH/JUMP FUNCTIONS ************************/
/**
 * @brief moves PC to current PC +/- the given offset
 *
 * @param cpu
 * @param value offset to move PC by
 */
static inline void branch(State6502 *cpu, uint8_t offset);

/**
 * @brief jump to new location
 *
 * @param cpu
 * @param address address to jump to
 */
static inline void jump(State6502 *cpu, uint16_t address);

/************************ ARITHMETIC FUNCTIONS ************************/

/**
 * @brief add to accumulator with carry. carry bit set if result greater than
 * 255
 *
 * @param cpu
 * @param value value to add to the accumulator
 */
static inline void adc(State6502 *cpu, uint8_t value);

/**
 * @brief subtract from accumulator with borrow
 * subtracts using two's complement arithmetic. carry bit set if the result is
 * greater or than equal to zero.
 * @param cpu
 * @param value value to add to the accumulator
 */
static inline void sbc(State6502 *cpu, uint8_t value);

/**
 * @brief increments the y register
 *
 * @param cpu
 */
static inline void iny(State6502 *cpu);

/**
 * @brief decrements y-register
 *
 * @param cpu
 */
static inline void dey(State6502 *cpu);

/**
 * @brief increments x-register
 *
 * @param cpu
 */
static inline void inx(State6502 *cpu);

/**
 * @brief decrements x-register
 *
 * @param cpu
 */
static inline void dex(State6502 *cpu);

/**
 * @brief increments memory
 *
 * @param cpu
 * @param address address of memory to increment
 */
static inline void inc(State6502 *cpu, uint16_t address);

/**
 * @brief decrements memory
 *
 * @param cpu
 * @param address address of memory to decrement
 */
static inline void dec(State6502 *cpu, uint16_t address);

/************************ ILLEGAL/MISCELLANEOUS ************************/

/**
 * @brief asl + or_a
 *
 * @param cpu
 * @param address
 */
static inline void slo(State6502 *cpu, uint16_t address);

/**
 * @brief rol + and
 *
 * @param cpu
 * @param address
 */
static inline void rla(State6502 *cpu, uint16_t address);

/**
 * @brief and + set carry
 *
 * @param cpu
 * @param address
 */
static inline void anc(State6502 *cpu, uint8_t value);

/**
 * @brief and + lsr
 *
 * @param cpu
 * @param value
 */
static inline void alr(State6502 *cpu, uint8_t value);

/**
 * @brief lsr + xor
 *
 * @param cpu
 * @param address
 */
static inline void sre(State6502 *cpu, uint16_t address);

/**
 * @brief ror + adc
 *
 * @param cpu
 * @param address
 */
static inline void rra(State6502 *cpu, uint16_t address);

/**
 * @brief magic
 * 
 * @param cpu 
 * @param value 
 */
static inline void ane(State6502 *cpu, uint8_t value);

/**
 * @brief and + ror
 * 
 * @param cpu 
 * @param value 
 */
static inline void arr(State6502 *cpu, uint8_t value);

/**
 * @brief ahx + axa
 *
 * @param cpu
 */
static inline void sha(State6502 *cpu, uint16_t address);

/**
 * @brief and + store in memory
 *
 * @param cpu
 * @param address
 */
static inline void sax(State6502 *cpu, uint16_t address);

/**
 * @brief set stack pointer to a & x, then SHY
 * 
 * @param cpu 
 * @param address 
 */
static inline void tas(State6502 *cpu, uint16_t address);

/**
 * @brief x & (address high byte + 1) -> memory[address]
 * 
 * @param cpu 
 * @param address 
 */
static inline void shx(State6502 *cpu, uint16_t address);

/**
 * @brief lda + ldx
 *
 * @param cpu
 * @param address
 */
static inline void lax(State6502 *cpu, uint8_t value);

/**
 * @brief lda/tsx
 * 
 * @param cpu 
 * @param address 
 */
static inline void las(State6502 *cpu, uint16_t address);

/**
 * @brief dec + cmp
 *
 * @param cpu
 * @param address
 */
static inline void dcp(State6502 *cpu, uint16_t address);

/**
 * @brief inc + sbc
 *
 * @param cpu
 * @param address
 */
static inline void isc(State6502 *cpu, uint16_t address);


/************************ EMULATION ************************/

int emulate6502Op(State6502 *cpu, uint8_t *opcode);

/**
 * @brief perform one cpu clock cycle
 * 
 * @param cpu 
 */
void clock_cpu(State6502 *cpu);