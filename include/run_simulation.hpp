
#ifndef RUN_SIMULATION_H
#define RUN_SIMULATION_H
#include "relevant_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Result run_simulation(
    uint32_t cycles,
    const char* tracefile,
    uint8_t endianness,
    uint32_t latencyScrambling,
    uint32_t latencyEncrypt,
    uint32_t latencyMemory_access,
    uint32_t seed,
    uint32_t numRequests,
    struct Request* requests
);

#ifdef __cplusplus
}
#endif

#endif // RUN_SIMULATION_H