#ifndef RELEVANT_STRUCTS_H
#define RELEVANT_STRUCTS_H

#include <stdint.h>

struct Result {
    uint32_t cycles;
    uint32_t errors;
    uint32_t primitiveGateCount;
};

struct Request {
    uint32_t addr;
    uint32_t data;
    uint8_t r;
    uint8_t w;
    uint32_t fault;
    uint8_t faultBit;
};

#endif 