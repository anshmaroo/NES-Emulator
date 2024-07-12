#include <stdio.h>
#include "mapper.hpp"

Mapper::Mapper(char *game, uint8_t mapper_number, uint8_t *buffer, Bus *bus) {
    this->game = (char *) malloc(sizeof(char) * 200);
    this->game = game;
    this->mapper_number = mapper_number;
    this->allow_cpu_writes = true;
    this->buffer = buffer;
    this->bus = bus;
}
