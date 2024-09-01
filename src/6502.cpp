#include "6502.h"

#include "2C02.h"
#include "Disassemble6502.h"
#include "bus.hpp"

/************************ CREATE OBJECT ************************/

uint16_t stack_val = 0;
uint8_t prev_sp = 0;

/**
 * @brief creates a 6502 object
 *
 * @return State6502*
 */
State6502 *Init6502(void) {
    State6502 *cpu = (State6502 *)malloc(sizeof(State6502));

    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;

    cpu->sr.n = 0;
    cpu->sr.v = 0;
    cpu->sr.b = 0;
    cpu->sr.d = 0;
    cpu->sr.i = 0;
    cpu->sr.z = 0;
    cpu->sr.c = 0;

    cpu->sp = 0xff;
    cpu->pc = 0;

    cpu->int_enable = 0;
    cpu->halted = 0;
    cpu->cycles = 0;

    cpu->bus = NULL;

    return cpu;
}

/************************ ADDRESSING MODES ************************/

uint16_t zero_page(State6502 *cpu, uint8_t *opcode) {
    return opcode[1];
}

uint16_t zero_page_x(State6502 *cpu, uint8_t *opcode) {
    uint8_t address;
    if ((opcode[1] >> 7) & 1) {
        address = (cpu->x - (~opcode[1] + 1)) & 0xff;
    }

    else {
        address = (cpu->x + opcode[1]) & 0xff;
    }

    return address;
}

uint16_t zero_page_y(State6502 *cpu, uint8_t *opcode) {
    uint8_t address;
    if ((opcode[1] >> 7) & 1) {
        address = (cpu->y - (~opcode[1] + 1)) & 0xff;
    }

    else {
        address = (cpu->y + opcode[1]) & 0xff;
    }

    return address;
}

uint16_t absolute(State6502 *cpu, uint8_t *opcode) {
    return (opcode[2] << 8) | (opcode[1]);
}

uint16_t absolute_x(State6502 *cpu, uint8_t *opcode) {
    uint16_t address = (cpu->x + (opcode[2] << 8 | opcode[1]));
    if ((address & 0xFF00) != (opcode[2] << 8))
        cpu->cycles++;

    return address;
}

uint16_t absolute_y(State6502 *cpu, uint8_t *opcode) {
    uint16_t address = (cpu->y + (opcode[2] << 8 | opcode[1]));
    if ((address & 0xFF00) != (opcode[2] << 8))
        cpu->cycles++;

    return address;
}

uint16_t x_indexed_indirect(State6502 *cpu, uint8_t *opcode) {
    uint8_t index = (opcode[1] + cpu->x) & 0xff;
    return (cpu_read_from_bus(cpu->bus, (index + 1) & 0xff) << 8) | cpu_read_from_bus(cpu->bus, index);
}

uint16_t indirect_y_indexed(State6502 *cpu, uint8_t *opcode) {
    uint16_t t = opcode[1];
    uint16_t lo = cpu_read_from_bus(cpu->bus, t & 0x00FF);
    uint16_t hi = cpu_read_from_bus(cpu->bus, (t + 1) & 0x00FF);

    uint16_t address = (hi << 8) | lo;
    address += cpu->y;

    if ((address & 0xFF00) != (hi << 8))
        cpu->cycles++;

    return address;
}

/************************ STATUS REGISTER FUNCTIONS ************************/
/**
 * @brief Get all the status register flags in one byte
 *
 * @param cpu
 * @return uint8_t
 */
static inline uint8_t get_status_register(State6502 *cpu) {
    uint8_t status_register = 0;

    status_register |= (cpu->sr.n << 7) & 0xff;
    status_register |= (cpu->sr.v << 6) & 0xff;
    status_register |= (1 << 5) & 0xff;
    status_register |= (cpu->sr.b << 4) & 0xff;
    status_register |= (cpu->sr.d << 3) & 0xff;
    status_register |= (cpu->sr.i << 2) & 0xff;
    status_register |= (cpu->sr.z << 1) & 0xff;
    status_register |= (cpu->sr.c << 0) & 0xff;

    return status_register;
}

/**
 * @brief set the status register flags from one byte
 *
 * @param cpu
 * @param byte
 */
static inline void set_status_register(State6502 *cpu, uint8_t byte) {
    // printf("STATUS REGISTER TO BET SET TO = %02x\n", byte);
    cpu->sr.c = (byte >> 0) & 0x1;
    cpu->sr.z = (byte >> 1) & 0x1;
    cpu->sr.i = (byte >> 2) & 0x1;
    cpu->sr.d = (byte >> 3) & 0x1;
    cpu->sr.b = (byte >> 4) & 0x0;
    cpu->sr.v = (byte >> 6) & 0x1;
    cpu->sr.n = (byte >> 7) & 0x1;
}

/**
 * @brief updates the zero and negative flags based on the result of the
 * previous operation
 *
 * @param cpu
 * @param value the result of an operation to updates flags based on
 */
static inline void update_zn(State6502 *cpu, uint8_t value) {
    cpu->sr.z = (value == 0) ? 1 : 0;  // zero
    cpu->sr.n = (value >> 7) ? 1 : 0;  // negative
}

/**
 * @brief set negative and overflow flags to bits 7 and 6 of the value provided.
 * sets the zero flag by ANDing with A register
 *
 * @param cpu
 * @param value value provided
 */
static inline void bit(State6502 *cpu, uint8_t value) {
    cpu->sr.z = (cpu->a & value) == 0;
    cpu->sr.n = value >> 7 & 0x1;
    cpu->sr.v = value >> 6 & 0x1;
}

/************************ LOGICAL FUNCTIONS ************************/
/**
 * @brief logical OR operation with the accumulator
 *
 * @param cpu
 * @param value value to OR with a
 */
static inline void or_a(State6502 *cpu, uint8_t value) {
    cpu->a |= value;
    update_zn(cpu, cpu->a);
}

/**
 * @brief logical AND operation with the accumulator
 *
 * @param cpu
 * @param value value to AND with a
 */
static inline void and_a(State6502 *cpu, uint8_t value) {
    cpu->a &= value;
    update_zn(cpu, cpu->a);
}

/**
 * @brief exclusive or with the accumulator
 *
 * @param cpu
 * @param value value to XOR with accumulator
 */
static inline void xor_a(State6502 *cpu, uint8_t value) {
    cpu->a ^= value;
    update_zn(cpu, cpu->a);
}

/**
 * @brief rotate value in memory left by one bit
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void asl_memory(State6502 *cpu, uint16_t address) {
    cpu->sr.c = cpu_read_from_bus(cpu->bus, address) >> 7 & 0x1;
    cpu_write_to_bus(cpu->bus, address, cpu_read_from_bus(cpu->bus, address) << 1);
    update_zn(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief rotate value in accumulator left by one bit
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void asl_a(State6502 *cpu) {
    cpu->sr.c = cpu->a >> 7 & 0x1;
    cpu->a = cpu->a << 1;
    update_zn(cpu, cpu->a);
}

/**
 * @brief shifts a value in memory one bit to the left
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void rol_memory(State6502 *cpu, uint16_t address) {
    bool cy = (cpu_read_from_bus(cpu->bus, address) >> 7) & 0x1;
    cpu_write_to_bus(cpu->bus, address, cpu_read_from_bus(cpu->bus, address) << 1 | cpu->sr.c);
    cpu->sr.c = cy;
    update_zn(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief shifts the value in the accumualtor one bit to the left
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void rol_a(State6502 *cpu) {
    bool cy = cpu->a >> 7 & 0x1;
    cpu->a = cpu->a << 1 | cpu->sr.c;
    cpu->sr.c = cy;
    update_zn(cpu, cpu->a);
}

/**
 * @brief shifts the value one bit to the right
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void ror_memory(State6502 *cpu, uint16_t address) {
    bool cy = (cpu_read_from_bus(cpu->bus, address)) & 0x1;
    cpu_write_to_bus(cpu->bus, address, cpu_read_from_bus(cpu->bus, address) >> 1 | (cpu->sr.c << 7));
    cpu->sr.c = cy;
    update_zn(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief shifts the value in the accumulator one bit to the right
 *
 * @param cpu
 */
static inline void ror_a(State6502 *cpu) {
    bool cy = cpu->a & 0x1;
    cpu->a = cpu->a >> 1 | (cpu->sr.c << 7);
    cpu->sr.c = cy;
    update_zn(cpu, cpu->a);
}

/**
 * @brief shift value in memory right by one bit
 *
 * @param cpu
 * @param address address of the value to shift
 */
static inline void lsr_memory(State6502 *cpu, uint16_t address) {
    cpu->sr.c = cpu_read_from_bus(cpu->bus, address) & 0x1;
    cpu_write_to_bus(cpu->bus, address, cpu_read_from_bus(cpu->bus, address) >> 1);
    update_zn(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief shift value in accumulator right by one bit
 *
 * @param cpu
 */
static inline void lsr_a(State6502 *cpu) {
    cpu->sr.c = cpu->a & 0x1;
    cpu->a = cpu->a >> 1;
    update_zn(cpu, cpu->a);
}

/**
 * @brief compares two values and updates flags based on the result
 *
 * @param cpu
 * @param register_value  value of the register to compare to
 * @param value value that is to be compared with
 */
static inline void cmp(State6502 *cpu, uint8_t register_value,
                       uint8_t value) {
    update_zn(cpu, register_value - value);
    cpu->sr.c = (value <= register_value) ? 1 : 0;
}

/************************ LOAD/STORE/MOVE FUNCTIONS ************************/
/**
 * @brief pushes an 8-bit value to the stack
 * Decreases the stack pointer by one byte, and stores an 8-bit value at the new
 * location of the stack
 * @param cpu
 * @param value the value to be pushed to the stack
 */
static inline void push(State6502 *cpu, uint8_t value) {
    cpu_write_to_bus(cpu->bus, (0x0100) | (cpu->sp), value);
    cpu->sp--;
    stack_val = value;
}

/**
 * @brief pops a value that was stored on the stack
 * Gets the value stored on the stack and increases the stack pointer by one
 * byte, resetting its original position.
 * @param cpu
 */
static uint8_t pop(State6502 *cpu) {
    cpu->sp++;
    uint8_t value = cpu_read_from_bus(cpu->bus, (0x0100 | cpu->sp));
    return value;
}

/**
 * @brief pushes a 16-bit value to the stack
 * Decreases the stack pointer by two bytes, and stores a 16-bit value at the
 * new location of the stack
 * @param cpu the State6502 object
 * @param value the value to be pushed to the stack
 */
static inline void push_16(State6502 *cpu, uint16_t value) {
    push(cpu, (value >> 8) & 0xff);
    // printf("PUSHED %02x TO %04X\n", value & 0xff, (0x100 | cpu->sp + 1));
    push(cpu, (value & 0xff));
    // printf("PUSHED %02x TO %04X\n", (value >> 8) & 0xff, (0x100 | cpu->sp + 1));
    stack_val = value;
}

/**
 * @brief pops a 16-bit value that was stored on the stack
 * Gets the value stored on the stack and increases the stack pointer by two
 * bytes, resetting its original position.
 * @param cpu the State6502 object
 */
static uint16_t pop_16(State6502 *cpu) {
    uint8_t low = pop(cpu);
    // printf("POPPED %02x FROM %04X\n", high, (0x100 | cpu->sp));
    uint8_t high = pop(cpu);
    // printf("POPPED %02x FROM %04X\n", low, (0x100 | cpu->sp));
    uint16_t value = (high << 8) | low;
    return value;
}

/**
 * @brief stores accumulator in memory
 *
 * @param cpu
 * @param address address to store accumulator at
 */
static inline void sta(State6502 *cpu, uint16_t address) {
    cpu_write_to_bus(cpu->bus, address, cpu->a);
}

/**
 * @brief stores y-register in memory
 *
 * @param cpu
 * @param address address to store accumulator at
 */
static inline void sty(State6502 *cpu, uint16_t address) {
    cpu_write_to_bus(cpu->bus, address, cpu->y);
}

/**
 * @brief stores x-register in memory
 *
 * @param cpu
 * @param address address to store accumulator at
 */
static inline void stx(State6502 *cpu, uint16_t address) {
    cpu_write_to_bus(cpu->bus, address, cpu->x);
}

/**
 * @brief transfers x-register to accumulator
 *
 * @param cpu
 */
static inline void txa(State6502 *cpu) {
    cpu->a = cpu->x;
    update_zn(cpu, cpu->a);
}

/**
 * @brief transfers accumulator to x-register
 *
 * @param cpu
 */
static inline void tax(State6502 *cpu) {
    cpu->x = cpu->a;
    update_zn(cpu, cpu->x);
}

/**
 * @brief transfers y-register to accumulator
 *
 * @param cpu
 */
static inline void tya(State6502 *cpu) {
    cpu->a = cpu->y;
    update_zn(cpu, cpu->a);
}

/**
 * @brief transfers accumulator to y-register
 * @param cpu
 */
static inline void tay(State6502 *cpu) {
    cpu->y = cpu->a;
    update_zn(cpu, cpu->y);
}

/**
 * @brief transfers stack pointer to x-register
 * @param cpu
 */
static inline void tsx(State6502 *cpu) {
    cpu->x = cpu->sp;
    update_zn(cpu, cpu->x);
}

/**
 * @brief transfers x-register to stack pointer
 * @param cpu
 */
static inline void txs(State6502 *cpu) {
    cpu->sp = cpu->x;
}

/**
 * @brief loads y-register with a value
 *
 * @param cpu
 * @param value value to load y-register with
 */
static inline void ldy(State6502 *cpu, uint8_t value) {
    // printf("value to set y to = %d\n", value);
    cpu->y = value;
    update_zn(cpu, cpu->y);
}

/**
 * @brief loads x-register with a value
 *
 * @param cpu
 * @param value value to load x-register with
 */
static inline void ldx(State6502 *cpu, uint8_t value) {
    cpu->x = value;
    update_zn(cpu, cpu->x);
}

/**
 * @brief loads accumulator with a value
 *
 * @param cpu
 * @param value value to load accumulator with
 */
static inline void lda(State6502 *cpu, uint8_t value) {
    cpu->a = value;
    update_zn(cpu, cpu->a);
}

/************************ I/O FUNCTIONS ************************/

/**
 * @brief set pc to reset vector
 *
 * @param cpu
 */
void reset(State6502 *cpu) {
    cpu->pc = (cpu_read_from_bus(cpu->bus, 0xfffd) << 8) | (cpu_read_from_bus(cpu->bus, 0xfffc));

    cpu->pc = (cpu_read_from_bus(cpu->bus, 0xfffd) << 8) | cpu_read_from_bus(cpu->bus, 0xfffc);
    cpu->cycles += 7;
}

/**
 * @brief interrupt from fffe and ffff
 *
 * @param cpu
 */
void irq(State6502 *cpu) {
    push_16(cpu, cpu->pc);
    push(cpu, get_status_register(cpu));
    cpu->sr.i = 1;
    cpu->pc = (cpu_read_from_bus(cpu->bus, 0xffff) << 8) | cpu_read_from_bus(cpu->bus, 0xfffe);
    cpu->cycles += 7;
}

/**
 * @brief interrupt an interrupt
 *
 * @param cpu
 */
void nmi(State6502 *cpu) {
    push_16(cpu, cpu->pc);
    push(cpu, get_status_register(cpu));
    cpu->pc = (cpu_read_from_bus(cpu->bus, 0xfffb) << 8) | cpu_read_from_bus(cpu->bus, 0xfffa);
    cpu->cycles += 7;
}

/************************ BRANCH/JUMP FUNCTIONS ************************/
/**
 * @brief moves PC to current PC +/- the given offset
 *
 * @param cpu
 * @param value offset to move PC by
 */
static inline void branch(State6502 *cpu, uint8_t offset) {
    if ((offset >> 7) & 1) {
        offset = ~offset + 1;
        cpu->pc = cpu->pc - offset;
    } else {
        cpu->pc = cpu->pc + offset;  // negative represented by two's complement
    }
}

/**
 * @brief jump to new location
 *
 * @param cpu
 * @param address address to jump to
 */
static inline void jump(State6502 *cpu, uint16_t address) {
    cpu->pc = address;
}

/************************ ARITHMETIC FUNCTIONS ************************/

/**
 * @brief add to accumulator with carry. carry bit set if result greater than
 * 255
 *
 * @param cpu
 * @param value value to add to the accumulator
 */
static inline void adc(State6502 *cpu, uint8_t value) {
    uint16_t sum = cpu->a + value + cpu->sr.c;
    cpu->sr.c = sum > 0xFF;
    // The overflow flag is set when the sign of the addends is the same and
    // differs from the sign of the sum
    cpu->sr.v = ~(cpu->a ^ value) & (cpu->a ^ sum) & 0x80;

    update_zn(cpu, sum);
    cpu->a = sum;
}

/**
 * @brief subtract from accumulator with borrow
 * subtracts using two's complement arithmetic. carry bit set if the result is
 * greater or than equal to zero.
 * @param cpu
 * @param value value to add to the accumulator
 */
static inline void sbc(State6502 *cpu, uint8_t value) {
    adc(cpu, ~value);
}

/**
 * @brief increments the y register
 *
 * @param cpu
 */
static inline void iny(State6502 *cpu) {
    cpu->y++;
    update_zn(cpu, cpu->y);
}

/**
 * @brief decrements y-register
 *
 * @param cpu
 */
static inline void dey(State6502 *cpu) {
    cpu->y--;
    update_zn(cpu, cpu->y);
}

/**
 * @brief increments x-register
 *
 * @param cpu
 */
static inline void inx(State6502 *cpu) {
    cpu->x++;
    update_zn(cpu, cpu->x);
}

/**
 * @brief decrements x-register
 *
 * @param cpu
 */
static inline void dex(State6502 *cpu) {
    cpu->x--;
    update_zn(cpu, cpu->x);
}

/**
 * @brief increments memory
 *
 * @param cpu
 * @param address address of memory to increment
 */
static inline void inc(State6502 *cpu, uint16_t address) {
    cpu_write_to_bus(cpu->bus, address, cpu_read_from_bus(cpu->bus, address) + 1);
    update_zn(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief decrements memory
 *
 * @param cpu
 * @param address address of memory to decrement
 */
static inline void dec(State6502 *cpu, uint16_t address) {
    cpu_write_to_bus(cpu->bus, address, cpu_read_from_bus(cpu->bus, address) - 1);
    update_zn(cpu, cpu_read_from_bus(cpu->bus, address));
}

/************************ ILLEGAL/MISCELLANEOUS ************************/

/**
 * @brief asl + or_a
 *
 * @param cpu
 * @param address
 */
static inline void slo(State6502 *cpu, uint16_t address) {
    asl_memory(cpu, address);
    or_a(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief rol + and
 *
 * @param cpu
 * @param address
 */
static inline void rla(State6502 *cpu, uint16_t address) {
    rol_memory(cpu, address);
    and_a(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief and + set carry
 *
 * @param cpu
 * @param address
 */
static inline void anc(State6502 *cpu, uint8_t value) {
    and_a(cpu, value);
    cpu->sr.c = 1;
}

/**
 * @brief and + lsr
 *
 * @param cpu
 * @param value
 */
static inline void alr(State6502 *cpu, uint8_t value) {
    and_a(cpu, value);
    lsr_a(cpu);
}

/**
 * @brief lsr + xor
 *
 * @param cpu
 * @param address
 */
static inline void sre(State6502 *cpu, uint16_t address) {
    lsr_memory(cpu, address);
    xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief ror + adc
 *
 * @param cpu
 * @param address
 */
static inline void rra(State6502 *cpu, uint16_t address) {
    ror_memory(cpu, address);
    adc(cpu, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief magic
 *
 * @param cpu
 * @param value
 */
static inline void ane(State6502 *cpu, uint8_t value) {
    cpu->a = (cpu->a | rand()) & cpu->x & value;
}

/**
 * @brief and + ror
 *
 * @param cpu
 * @param value
 */
static inline void arr(State6502 *cpu, uint8_t value) {
    and_a(cpu, value);
    ror_a(cpu);
}

/**
 * @brief ahx + axa
 *
 * @param cpu
 */
static inline void sha(State6502 *cpu, uint16_t address) {
    uint8_t value = cpu_read_from_bus(cpu->bus, (address >> 8) & 0xff + 1) & cpu->a & cpu->x;
    cpu_write_to_bus(cpu->bus, address, value);
}

/**
 * @brief and + store in memory
 *
 * @param cpu
 * @param address
 */
static inline void sax(State6502 *cpu, uint16_t address) {
    cpu_write_to_bus(cpu->bus, address, cpu->a & cpu->x);
}

/**
 * @brief set stack pointer to a & x, then SHY
 *
 * @param cpu
 * @param address
 */
static inline void tas(State6502 *cpu, uint16_t address) {
    cpu->sp = cpu->a & cpu->x;
    sha(cpu, address);
}

/**
 * @brief x & (address high byte + 1) -> memory[address]
 *
 * @param cpu
 * @param address
 */
static inline void shx(State6502 *cpu, uint16_t address) {
    uint8_t value = cpu->x & cpu_read_from_bus(cpu->bus, (address >> 8) & 0xff + 1);
    cpu_write_to_bus(cpu->bus, address, value);
}

/**
 * @brief lda + ldx
 *
 * @param cpu
 * @param address
 */
static inline void lax(State6502 *cpu, uint8_t value) {
    lda(cpu, value);
    ldx(cpu, value);
}

/**
 * @brief lda/tsx
 *
 * @param cpu
 * @param address
 */
static inline void las(State6502 *cpu, uint16_t address) {
    uint8_t value = cpu_read_from_bus(cpu->bus, address) & cpu->sp;
    lda(cpu, value);
    ldx(cpu, value);
    cpu->sp = value;
}

/**
 * @brief dec + cmp
 *
 * @param cpu
 * @param address
 */
static inline void dcp(State6502 *cpu, uint16_t address) {
    dec(cpu, address);
    cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
}

/**
 * @brief inc + sbc
 *
 * @param cpu
 * @param address
 */
static inline void isc(State6502 *cpu, uint16_t address) {
    inc(cpu, address);
    sbc(cpu, cpu_read_from_bus(cpu->bus, address));
}

/************************ EMULATION ************************/

int emulate6502Op(State6502 *cpu, uint8_t *opcode) {
    if (DEBUG) {
        printf("%s\n", Disassemble6502Op(opcode, cpu->pc));
    }

    switch (*opcode) {
        case 0x00:  // BRK imp
        {
            push_16(cpu, cpu->pc + 2);  // push PC + 2 to the stack
            uint8_t brk_sr = get_status_register(cpu) ^ 0x10;
            push(cpu, brk_sr);
            cpu->sr.i = 1;
            cpu->pc = (cpu_read_from_bus(cpu->bus, 0xffff) << 8) | (cpu_read_from_bus(cpu->bus, 0xfffe));

            break;
        }

        case 0x01:  // or_a, (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x02:  // JAM (KIL, HLT)
        {
            break;
        }

        case 0x03:  // SLO (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            slo(cpu, address);
            break;
        }

        case 0x04:  // NOP
        {
            break;
        }

        case 0x05:  // or_a, $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x06:  // ASL, $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            asl_memory(cpu, address);
            break;
        }

        case 0x07:  // SLO $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            slo(cpu, address);
            break;
        }

        case 0x08:  // PHP (implied)
        {
            uint8_t brk_sr = get_status_register(cpu) ^ 0x10;
            push(cpu, brk_sr);
            break;
        }

        case 0x09:  // or_a, #$oper (immediate)
        {
            or_a(cpu, opcode[1]);
            break;
        }

        case 0x0a:  // ASL, A (implied)
        {
            asl_a(cpu);
            break;
        }

        case 0x0b:  // ANC #$oper (immediate)
        {
            and_a(cpu, opcode[1]);
            cpu->sr.c = 1;
            break;
        }

        case 0x0c:  // NOP
        {
            break;
        }

        case 0x0d:  // or_a, $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | (opcode[1]);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x0e:  // ASL, $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | (opcode[1]);
            asl_memory(cpu, address);
            break;
        }

        case 0x0f:  // SLO $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            slo(cpu, address);
            break;
        }

        case 0x10:  // BPL $oper (relative)
        {
            if (cpu->sr.n == 0) {  // check if result was positive
                branch(cpu, opcode[1]);
            }
            break;
        }

        case 0x11:  // or_a ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x12:  // JAM
        {
            break;
        }

        case 0x13:  // SLO ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            slo(cpu, address);
            break;
        }

        case 0x14:  // NOP $oper, X (zero-page, x-indexed)
        {
            break;
        }

        case 0x15:  // or_a, $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x16:  // ASL, $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            asl_memory(cpu, address);
            break;
        }

        case 0x17:  // SLO, $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            slo(cpu, address);
            break;
        }

        case 0x18:  // CLC (implied)
        {
            cpu->sr.c = 0;
            break;
        }

        case 0x19:  // ORA Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x1a:  // NOP
        {
            break;
        }

        case 0x1b:  // SLO Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            slo(cpu, address);
            break;
        }

        case 0x1c:  // NOP X, $oper $oper (absolute, x-indexed)
        {
            break;
        }

        case 0x1d:  // or_a X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            or_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x1e:  // ASL X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            asl_memory(cpu, address);
            break;
        }

        case 0x1f:  // SLO Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            slo(cpu, address);
            break;
        }

        case 0x20:  // JSR $oper $oper (absolute)
        {
            push_16(cpu, cpu->pc + 2);
            uint16_t address = absolute(cpu, opcode);
            cpu->pc = address;
            break;
        }

        case 0x21:  // AND (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x22:  // JAM
        {
            break;
        }

        case 0x23:  // RLA (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x24:  // BIT $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            bit(cpu, cpu_read_from_bus(cpu->bus, opcode[1]));
            break;
        }

        case 0x25:  // AND $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x26:  // ROL $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            rol_memory(cpu, address);
            break;
        }

        case 0x27:  // RLA $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x28:  // PLP (implied)
        {
            set_status_register(cpu, pop(cpu));
            break;
        }

        case 0x29:  // AND #$oper (immediate)
        {
            and_a(cpu, opcode[1]);
            break;
        }

        case 0x2a:  // ROL A (implied)
        {
            rol_a(cpu);
            break;
        }

        case 0x2b:  // ANC #$oper (immediate)
        {
            anc(cpu, cpu_read_from_bus(cpu->bus, opcode[1]));
            break;
        }

        case 0x2c:  // BIT $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            bit(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x2d:  // AND  $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x2e:  // ROL $oper $oper (asbsolute)
        {
            uint16_t address = absolute(cpu, opcode);
            rol_memory(cpu, address);
            break;
        }

        case 0x2f:  // RLA $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x30:  // BMI (relative)
        {
            if (cpu->sr.n == 1)
                branch(cpu, opcode[1]);
            break;
        }

        case 0x31:  // AND ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x32:  // JAM
        {
            break;
        }

        case 0x33:  // RLA ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x34:  // NOP
        {
            break;
        }

        case 0x35:  // AND $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x36:  // ROL $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            rol_memory(cpu, address);
            break;
        }

        case 0x37:  // RLA $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x38:  // SEC
        {
            cpu->sr.c = 1;
            break;
        }

        case 0x39:  // AND Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x3a:  // NOP
        {
            break;
        }

        case 0x3b:  // RLA Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x3c:  // NOP
        {
            break;
        }

        case 0x3d:  // AND X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            and_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x3e:  // ROL X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            rol_memory(cpu, address);
            break;
        }

        case 0x3f:  // RLA X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            rla(cpu, address);
            break;
        }

        case 0x40:  // RTI
        {
            set_status_register(cpu, pop(cpu));
            cpu->pc = pop_16(cpu);
            break;
        }

        case 0x41:  // EOR (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x42:  // JAM
        {
            break;
        }

        case 0x43:  // SRE
        {
            uint8_t index = (opcode[1] + cpu->x) & 0xff;
            uint16_t address = (cpu_read_from_bus(cpu->bus, (index + 1) & 0xff) << 8) | cpu_read_from_bus(cpu->bus, index);
            sre(cpu, address);
            break;
        }

        case 0x44:  // NOP
        {
            break;
        }

        case 0x45:  // EOR $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x46:  // LSR $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            lsr_memory(cpu, address);
            break;
        }

        case 0x47:  // SRE (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            sre(cpu, address);
        }

        case 0x48:  // PHA (implied)
        {
            push(cpu, cpu->a);
            break;
        }

        case 0x49:  // EOR #$oper (immediate)
        {
            xor_a(cpu, opcode[1]);
            break;
        }

        case 0x4a:  // LSR A (implied)
        {
            lsr_a(cpu);
            break;
        }

        case 0x4b:  // ALR #$oper (immediate)
        {
            alr(cpu, opcode[1]);
            break;
        }

        case 0x4c:  // JMP $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            jump(cpu, address);
            break;
        }

        case 0x4d:  // EOR $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x4e:  // LSR $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            lsr_memory(cpu, address);
            break;
        }

        case 0x4f:  // SRE $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            sre(cpu, address);
            break;
        }

        case 0x50:  // BVC $oper (relative)
        {
            if (cpu->sr.v == 0)
                branch(cpu, opcode[1]);
            break;
        }

        case 0x51:  // EOR ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x52:  // JAM
        {
            break;
        }

        case 0x53:  // SRE ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            sre(cpu, address);
            break;
        }

        case 0x54:  // NOP
        {
            break;
        }

        case 0x55:  // EOR  $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x56:  // LSR $oper (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            lsr_memory(cpu, address);
            break;
        }

        case 0x57:  // SRE $oper (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            sre(cpu, address);
            break;
        }

        case 0x58:  // CLI (implied)
        {
            cpu->sr.i = 0;
            break;
        }

        case 0x59:  // EOR Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x5a:  // NOP
        {
            break;
        }

        case 0x5b:  // SRE Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            sre(cpu, address);
            break;
        }

        case 0x5c:  // NOP
        {
            break;
        }

        case 0x5d:  // EOR X, $oper, $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            xor_a(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x5e:  // LSR X, $oper, $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            lsr_memory(cpu, address);
            break;
        }

        case 0x5f:  // SRE, $oper, $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            sre(cpu, address);
            break;
        }

        case 0x60:  // RTS (implied)
        {
            uint16_t address = pop_16(cpu);
            cpu->pc = address;
            break;
        }

        case 0x61:  // ADC (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x62:  // JAM
        {
            break;
        }

        case 0x63:  // RRA (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x64:  // NOP
        {
            break;
        }

        case 0x65:  // ADC $oper (zero-paged)
        {
            uint16_t address = zero_page(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x66:  // ROR $oper (zero-paged)
        {
            uint16_t address = zero_page(cpu, opcode);
            ror_memory(cpu, address);
            break;
        }

        case 0x67:  // RRA (zero-paged)
        {
            uint16_t address = zero_page(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x68:  // PLA (implied)
        {
            cpu->a = pop(cpu);
            update_zn(cpu, cpu->a);
            break;
        }

        case 0x69:  // ADC #$oper (immediate)
        {
            adc(cpu, opcode[1]);
            break;
        }

        case 0x6a:  // ROR A (implied)
        {
            ror_a(cpu);
            break;
        }

        case 0x6b:  // ARR #$oper (immediate)
        {
            arr(cpu, opcode[1]);
            break;
        }

        case 0x6c:  // JMP ($oper $oper) (indirect)
        {
            uint16_t index = (opcode[2] << 8) | opcode[1];
            uint16_t address = (cpu_read_from_bus(cpu->bus, (index + 1)) << 8) | cpu_read_from_bus(cpu->bus, index);

            if ((index & 0xff) == 0xff)
                address = (cpu_read_from_bus(cpu->bus, (uint16_t)(index & 0xff00)) << 8) | cpu_read_from_bus(cpu->bus, index);

            jump(cpu, address);
            break;
        }

        case 0x6d:  // ADC $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x6e:  // ROR $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            ror_memory(cpu, address);
            break;
        }

        case 0x6f:  // RRA $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x70:  // BVS (relative)
        {
            if (cpu->sr.v == 1)
                branch(cpu, opcode[1]);
            break;
        }

        case 0x71:  // ADC ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x72:  // JAM
        {
            break;
        }

        case 0x73:  // RRA ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x74:  // NOP
        {
            break;
        }

        case 0x75:  // ADC $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x76:  // ROR $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            ror_memory(cpu, address);
            break;
        }

        case 0x77:  // RRA $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x78:  // SEI (implied)
        {
            cpu->sr.i = 1;
            break;
        }

        case 0x79:  // ADC Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x7a:  // NOP
        {
            break;
        }

        case 0x7b:  // RRA Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x7c:  // NOP
        {
            break;
        }

        case 0x7d:  // ADC X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            adc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0x7e:  // ROR X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            ror_memory(cpu, address);
            break;
        }

        case 0x7f:  // RRA X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            rra(cpu, address);
            break;
        }

        case 0x80:  // NOP
        {
            break;
        }

        case 0x81:  // STA X, $oper $oper (x-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x82:  // NOP
        {
            break;
        }

        case 0x83:  // SAX X, $oper $oper (x-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            sax(cpu, address);
            break;
        }

        case 0x84:  // STY $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            sty(cpu, address);
            break;
        }

        case 0x85:  // STA $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x86:  // STX $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            stx(cpu, address);
            break;
        }

        case 0x87:  // SAX $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            sax(cpu, address);
            break;
        }

        case 0x88:  // DEY (implied)
        {
            dey(cpu);
            break;
        }

        case 0x89:  // NOP
        {
            break;
        }

        case 0x8a:  // TXA (implied)
        {
            txa(cpu);
            break;
        }

        case 0x8b:  // ANE #$oper (immediate)
        {
            ane(cpu, opcode[1]);
        }

        case 0x8c:  // STY $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            sty(cpu, address);
            break;
        }

        case 0x8d:  // STA $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x8e:  // STX $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            stx(cpu, address);
            break;
        }

        case 0x8f:  // SAX $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            sax(cpu, address);
            break;
        }

        case 0x90:  // BCC (relative)
        {
            if (cpu->sr.c == 0)
                branch(cpu, opcode[1]);
            break;
        }

        case 0x91:  // STA ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x92:  // JAM
        {
            break;
        }

        case 0x93:  // SHA ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            sha(cpu, address);
            break;
        }

        case 0x94:  // STY $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            sty(cpu, address);
            break;
        }

        case 0x95:  // STA $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x96:  // STX $oper, Y (zero-page, y-indexed)
        {
            uint16_t address = zero_page_y(cpu, opcode);
            stx(cpu, address);
            break;
        }

        case 0x97:  // SAX $oper, Y (zero-page, y-indexed)
        {
            uint16_t address = zero_page_y(cpu, opcode);
            sax(cpu, address);
            break;
        }

        case 0x98:  // TYA (implied)
        {
            tya(cpu);
            break;
        }

        case 0x99:  // STA Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x9a:  // TXS (implied)
        {
            txs(cpu);
            break;
        }

        case 0x9b:  // TAS Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            tas(cpu, address);
            break;
        }

        case 0x9c:  // SHY X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x9d:  // STA X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            sta(cpu, address);
            break;
        }

        case 0x9e:  // SHX Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            shx(cpu, address);
            break;
        }

        case 0x9f:  // SHA  Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            sha(cpu, address);
            break;
        }

        case 0xa0:  // LDY #$oper (immediate)
        {
            ldy(cpu, opcode[1]);
            break;
        }

        case 0xa1:  // LDA (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xa2:  // LDX #$oper (immediate)
        {
            ldx(cpu, opcode[1]);
            break;
        }

        case 0xa3:  // LAX (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            lax(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xa4:  // LDY $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            ldy(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xa5:  // LDA $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xa6:  // LDX $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            ldx(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xa7:  // LAX $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            lax(cpu, cpu_read_from_bus(cpu->bus, address));
        }

        case 0xa8:  // TAY (implied)
        {
            tay(cpu);
            break;
        }

        case 0xa9:  // LDA #$oper (immediate)
        {
            lda(cpu, opcode[1]);
            break;
        }

        case 0xaa:  // TAX (implied)
        {
            tax(cpu);
            break;
        }

        case 0xab:  // LXA
        {
            lax(cpu, opcode[1]);
        }

        case 0xac:  // LDY $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            ldy(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xad:  // LDA $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xae:  // LDX $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            ldx(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xaf:  // LAX $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            lax(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb0:  // BCS (relative)
        {
            if (cpu->sr.c == 1)
                branch(cpu, opcode[1]);
            break;
        }

        case 0xb1:  // LDA ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb2:  // JAM
        {
            break;
        }

        case 0xb3:  // LAX ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            lax(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb4:  // LDY $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            ldy(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb5:  // LDA $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb6:  // LDX $oper, X (zero-page, y-indexed)
        {
            uint8_t address;
            if ((opcode[1] >> 7) & 1) {
                address = (cpu->y - (~opcode[1] + 1)) & 0xff;
            } else {
                address = (cpu->y + opcode[1]) & 0xff;
            }
            // address = (cpu->x + opcode[1]) & 0xff;

            // printf("address = %d\n", address);
            ldx(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb7:  // LAX $oper, X (zero-page, y-indexed)
        {
            uint8_t address;
            if ((opcode[1] >> 7) & 1) {
                address = (cpu->y - (~opcode[1] + 1)) & 0xff;
            } else {
                address = (cpu->y + opcode[1]) & 0xff;
            }
            // address = (cpu->x + opcode[1]) & 0xff;

            // printf("address = %d\n", address);
            lax(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xb8:  // CLV (implied)
        {
            cpu->sr.v = 0;
            break;
        }

        case 0xb9:  // LDA Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xba:  // TSX (implied)
        {
            tsx(cpu);
            break;
        }

        case 0xbb:  // LAS Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            las(cpu, address);
            break;
        }

        case 0xbc:  // LDY X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            ldy(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xbd:  // LDA X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            // printf("ADDRESS = $%04x\n", address);
            lda(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xbe:  // LDX Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            ldx(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xbf:  // LAX Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            lax(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xc0:  // CPY #$oper (immediate)
        {
            cmp(cpu, cpu->y, opcode[1]);
            break;
        }

        case 0xc1:  // CMP (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xc2:  // NOP
        {
            break;
        }

        case 0xc3:  // DCP (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xc4:  // CPY $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            cmp(cpu, cpu->y, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xc5:  // CMP $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xc6:  // DEC $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            dec(cpu, address);
            break;
        }

        case 0xc7:  // DCP $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xc8:  // INY (implied)
        {
            iny(cpu);
            break;
        }

        case 0xc9:  // CMP #$oper (immediate)
        {
            cmp(cpu, cpu->a, opcode[1]);
            break;
        }

        case 0xca:  // DEX (implied)
        {
            dex(cpu);
            break;
        }

        case 0xcb:  // DCP Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xcc:  // CPY $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            cmp(cpu, cpu->y, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xcd:  // CMP $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xce:  // DEC $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            dec(cpu, address);
            break;
        }

        case 0xcf:  // DCP // DEC $oper $oper (absolute)
        {
            uint16_t address = absolute(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xd0:  // BNE (relative)
        {
            if (cpu->sr.z == 0)
                branch(cpu, opcode[1]);
            break;
        }

        case 0xd1:  // CMP ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xd2:  // NOP
        {
            break;
        }

        case 0xd3:  // DCP ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xd4:  // NOP
        {
            break;
        }

        case 0xd5:  // CMP $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xd6:  // DEC $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            dec(cpu, address);
            break;
        }

        case 0xd7:  // DCP $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xd8:  // CLD (implied)
        {
            cpu->sr.d = 0;
            break;
        }

        case 0xd9:  // CMP Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xda:  // NOP
        {
            break;
        }

        case 0xdb:  // DCP Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xdc:  // NOP
        {
            break;
        }

        case 0xdd:  // CMP X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            cmp(cpu, cpu->a, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xde:  // DEC X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            dec(cpu, address);
            break;
        }

        case 0xdf:  // DCP X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            dcp(cpu, address);
            break;
        }

        case 0xe0:  // CPX #$oper (immediate)
        {
            cmp(cpu, cpu->x, opcode[1]);
            break;
        }

        case 0xe1:  // SBC (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xe2:  // NOP
        {
            break;
        }

        case 0xe3:  // ISC (X, $oper) (X-indexed, indirect)
        {
            uint16_t address = x_indexed_indirect(cpu, opcode);
            isc(cpu, address);
            break;
        }

        case 0xe4:  // CPX $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            cmp(cpu, cpu->x, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xe5:  // SBC $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xe6:  // INC $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            inc(cpu, address);
            break;
        }

        case 0xe7:  // ISC $oper (zero-page)
        {
            uint16_t address = zero_page(cpu, opcode);
            isc(cpu, address);
            break;
        }

        case 0xe8:  // INX (implied)
        {
            inx(cpu);
            break;
        }

        case 0xe9:  // SBC #$oper (immediate)
        {
            sbc(cpu, opcode[1]);
            break;
        }

        case 0xea:  // NOP (implied)
        {
            break;
        }

        case 0xeb:  // USBC #$oper (immediate)
        {
            sbc(cpu, opcode[1]);
        }

        case 0xec:  // CPX $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            cmp(cpu, cpu->x, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xed:  // SBC $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xee:  // INC $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            inc(cpu, address);
            break;
        }

        case 0xef:  // ISC $oper $oper (absolute)
        {
            uint16_t address = (opcode[2] << 8) | opcode[1];
            isc(cpu, address);
            break;
        }

        case 0xf0:  // BEQ $oper (relative)
        {
            if (cpu->sr.z == 1)
                branch(cpu, opcode[1]);
            break;
        }

        case 0xf1:  // SBC ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xf2:  // JAM
        {
            break;
        }

        case 0xf3:  // ISC ($oper), Y (indirect, Y-indexed)
        {
            uint16_t address = indirect_y_indexed(cpu, opcode);
            isc(cpu, address);
            break;
        }

        case 0xf4:  // NOP
        {
            break;
        }

        case 0xf5:  // SBC $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xf6:  // INC $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            inc(cpu, address);
            break;
        }

        case 0xf7:  // ISC $oper, X (zero-page, x-indexed)
        {
            uint16_t address = zero_page_x(cpu, opcode);
            isc(cpu, address);
            break;
        }

        case 0xf8:  // SED (implied)
        {
            cpu->sr.d = 1;
            break;
        }

        case 0xf9:  // SBC Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xfa:  // NOP
        {
            break;
        }

        case 0xfb:  // ISC Y, $oper $oper (absolute, y-indexed)
        {
            uint16_t address = absolute_y(cpu, opcode);
            isc(cpu, address);
            break;
        }

        case 0xfc:  // NOP
        {
            break;
        }

        case 0xfd:  // SBC X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            sbc(cpu, cpu_read_from_bus(cpu->bus, address));
            break;
        }

        case 0xfe:  // INC
        {
            uint16_t address = absolute_x(cpu, opcode);
            inc(cpu, address);
            break;
        }

        case 0xff:  // ISC X, $oper $oper (absolute, x-indexed)
        {
            uint16_t address = absolute_x(cpu, opcode);
            isc(cpu, address);
            break;
        }

        default: {
            printf("ERROR: UNIMPLEMENTED INSTRUCTION %02x.\n", opcode[0]);
            break;
        }
    }

    /* print out processor cpu */
    if (DEBUG) {
        printf("\tN=%d,V=%d,B=%d,D=%d,I=%d,Z=%d,C=%d\n", cpu->sr.n, cpu->sr.v,
               cpu->sr.b, cpu->sr.d, cpu->sr.i, cpu->sr.z, cpu->sr.c);
        printf("\tA $%02x X $%02x Y $%02x SP %04x PC %04x\n", cpu->a, cpu->x,
               cpu->y, cpu->sp, cpu->pc);
    }

    return 0;
}

/**
 * @brief perform one cpu clock cycle, update bytes and cycles
 *
 * @param cpu
 */
void clock_cpu(State6502 *cpu) {
    if (cpu->cycles == 0) {
        uint8_t opcode[3] = {cpu_read_from_bus(cpu->bus, cpu->pc), cpu_read_from_bus(cpu->bus, cpu->pc + 1), cpu_read_from_bus(cpu->bus, cpu->pc + 2)};

        emulate6502Op(cpu, opcode);

        cpu->cycles = OPCODES_CYCLES[opcode[0]];
        cpu->pc += OPCODES_BYTES[opcode[0]];
    }
    cpu->cycles--;
}