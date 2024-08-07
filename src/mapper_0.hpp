#include "mapper.hpp"

class Mapper_0 : public Mapper {
    public:

        Mapper_0(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus) : Mapper(game, mapper_number, buffer, bus) {

        }
        void initialize() override;
        
};