#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <stdint.h>

#include "memory.h"

typedef enum _arch
{
    ARCH_NONE = 0,

    ARCH_ORIGINAL,
    ARCH_NEW,
} arch_t;

typedef enum _error
{
    NO_ERROR = 0,

    ARG_NULL = -1,
    ALLOC_FAILED = -2,

    ARCH_UNSUPPORTED = 1,
} error_t;

typedef enum _inst_result
{
    SUCCESS,
    SYSCALL,
    ILLEGAL_INSTRUCTION,
} inst_result_t;

typedef struct _emulator
{
    memories_t memories;
    arch_t architecture;

    uint16_t current_syscall;

    // Registers
    address_t PC; // Original architecture
    address_t SP; // Original architecture
    address_t SR; // Original architecture
    address_t X; // New architecture
    uint8_t SI; // New architecture
    uint8_t CC; // Original architecture
    uint8_t DP; // New architecture
} emulator;

error_t init_emulator(emulator *emulator, arch_t architecture);
inst_result_t execute_instruction(emulator *emulator);
error_t dispose_emulator(emulator *emulator);

#endif // __EMULATOR_H__