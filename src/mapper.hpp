#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Bus;

#ifndef MAPPER_HPP
#define MAPPER_HPP
class Mapper {
    public:

        char *game;
        uint8_t mapper_number;

        uint16_t num_prg_banks;
        uint16_t num_chr_banks;

        uint16_t prg_bank_size;
        uint16_t chr_bank_size;

        bool allow_cpu_writes;
        bool chr_bank_switch;

        uint8_t *buffer;

        Bus *bus;

        Mapper() = default;
        Mapper(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus);
        virtual void initialize() {};
        virtual void handle_write(uint16_t address, uint8_t value) {};
        virtual void check_a12_rising_edge() {};
        virtual void cleanup() {};
        
};
#endif