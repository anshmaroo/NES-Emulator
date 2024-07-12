#include "mapper.hpp"

struct State2C02;

class Mapper_1 : public Mapper {
   public:
    Mapper_1(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus) : Mapper(game, mapper_number, buffer, bus) {
    }

    uint8_t load_counter;
    uint8_t load : 5;
    union control {
        struct {
            uint8_t mirror_mode : 2;
            uint8_t prg_bank_mode : 2;
            uint8_t chr_bank_mode : 1;
        };

        uint8_t reg : 5;
    } control;
    uint8_t chr_bank_0 : 5;
    uint8_t chr_bank_1 : 5;
    bool chr_bank_to_switch;
    union prg_bank {
        struct {
            uint8_t bank_select : 4;
            uint8_t ram_enable : 1;
        };

        uint8_t reg : 5;
    } prg_bank;

    void initialize() override;
    void handle_write(uint16_t address, uint8_t value);
    void switch_prg_bank();
    void switch_chr_bank();
    void cleanup() override;
};