#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

typedef uint16_t address_t;

#define DATA_SIZE           ((unsigned int)0x10000)
#define EXECUTE_BEGIN       ((address_t)0x0000)
#define STACK_SIZE          ((unsigned int)0x0100)
#define INST_STACK_SIZE     ((unsigned int)0x0100)

typedef struct _memories
{
    uint8_t *data;
    uint8_t *user_stack;
    uint8_t *instruction_stack;
} memories_t;

#endif // __MEMORY_H__