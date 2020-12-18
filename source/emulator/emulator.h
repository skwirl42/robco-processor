#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "memory.h"
#include "graphics.h"

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
    SYNC,
} inst_result_t;

typedef enum _execution_state
{
    RUNNING,
    FINISHED,
    WAITING,
    ERROR,
    DEBUGGING,
} execution_state_t;

typedef struct _emulator
{
    memories_t memories;
    arch_t architecture;

    uint16_t current_syscall;
    execution_state_t current_state;

    // Graphics will be in the 'data' memory
    address_t graphics_start;
    graphics_mode_t graphics_mode;

    // Registers
    address_t PC;
    address_t X;
    uint8_t SP;
    uint8_t ISP;
    uint8_t CC;
    uint8_t DP;
} emulator;

typedef union
{
    uint16_t word;
    uint8_t bytes[2];
} emulator_word_t;

typedef struct opcode_entry opcode_entry_t;

#define DEBUGGING_BUFFER_COUNT 5

error_t init_emulator(emulator *emulator, arch_t architecture);
void get_debug_info(emulator *emulator, char *debugging_buffers[DEBUGGING_BUFFER_COUNT]);
inst_result_t execute_instruction(emulator *emulator, opcode_entry_t **executed_instruction);
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