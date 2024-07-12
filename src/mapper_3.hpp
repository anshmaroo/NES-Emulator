#include "mapper.hpp"

struct State2C02;

class Mapper_3 : public Mapper {
    public:

        Mapper_3(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus) : Mapper(game, mapper_number, buffer, bus) {

        }
        void initialize() override;
        void handle_write(uint16_t address, uint8_t value) override;
        void switch_chr_bank(uint8_t chr_bank_number);
};

