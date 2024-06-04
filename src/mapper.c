#include "mapper.h"

MapperInterface *InitMapper() {
    MapperInterface *mapper = malloc(sizeof(MapperInterface));

    mapper->buffer = NULL;
    mapper->allow_cpu_writes = true;
    mapper->initialize = NULL;
    mapper->switch_prg_banks = NULL;

    return mapper;
}