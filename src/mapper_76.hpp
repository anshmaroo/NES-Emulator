#include "mapper.hpp"

struct State2C02;

class Mapper_76 : public Mapper {
   public:
    Mapper_76(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus) : Mapper(game, mapper_number, buffer, bus) {
    }

    uint8_t bank_address : 3;
    uint8_t data_port : 6;

    void initialize() override;
    void handle_write(uint16_t address, uint8_t value) override;
    void switch_prg_bank();
    void switch_chr_bank();
    void cleanup() override;
};
