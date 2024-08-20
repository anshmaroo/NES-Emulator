#include "mapper.hpp"

struct State2C02;

class Mapper_4 : public Mapper {
    public:

        Mapper_4(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus) : Mapper(game, mapper_number, buffer, bus) {

        }

        union bank_select {
            struct {
                uint8_t index : 3;
                uint8_t unused : 3;
                uint8_t prg_bank_mode : 1;
                uint8_t chr_inversion : 1;
            };

            uint8_t reg : 8;
        } bank_select;
        uint8_t bank_number;

        uint8_t mirroring;
        uint8_t prg_ram_protect;

        uint8_t irq_counter;
        uint8_t irq_latch;
        uint8_t irq_reload;
        uint8_t irq_disable;
        uint8_t irq_enable;
        int a12_low_counter;
        bool fire_irq;
        

        


        void initialize() override;
        void handle_write(uint16_t address, uint8_t value) override;
        void switch_prg_bank();
        void switch_chr_bank();
        void check_a12_rising_edge();
        void cleanup() override;
};

