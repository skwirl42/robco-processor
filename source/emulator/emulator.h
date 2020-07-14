#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

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
    EXECUTE_SYSCALL,
    ILLEGAL_INSTRUCTION,
} inst_result_t;

typedef enum _execution_state
{
    RUNNING,
    FINISHED,
    WAITING,
    ERROR,
} execution_state_t;

typedef struct _emulator
{
    memories_t memories;
    arch_t architecture;

    uint16_t current_syscall;
    execution_state_t current_state;

    // Registers
    address_t PC; // Original architecture
    address_t SP; // Original architecture
    address_t X; // New architecture
    uint8_t CC; // Original architecture
    uint8_t SI; // New architecture
    uint8_t DP; // New architecture
} emulator;

error_t init_emulator(emulator *emulator, arch_t architecture);
inst_result_t execute_instruction(emulator *emulator);
uint16_t pull_word(emulator *emulator);
uint8_t pull_byte(emulator *emulator);
void push_word(emulator *emulator, uint16_t word);
void push_byte(emulator *emulator, uint8_t byte);
void pop_bytes(emulator *emulator, uint16_t byte_count);
uint8_t emulator_can_execute(emulator *emulator);
error_t dispose_emulator(emulator *emulator);

#ifdef __cplusplus
}
#endif

#endif // __EMULATOR_H__