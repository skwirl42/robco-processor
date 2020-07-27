#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

typedef uint16_t address_t;

#define DATA_SIZE            ((unsigned int)0xC000)
#define STACK_BEGIN         ((address_t)0xFFFF)
#define STACK_SIZE          ((address_t)0x4000)
#define STACK_END           ((address_t)0xC000)
#define INST_SIZE           ((unsigned int)0x8000)
#define EXECUTE_BEGIN       ((address_t)0x0000)
#define INST_STACK_SIZE     ((unsigned int)0x4000)
#define INST_STACK_BEGIN    ((address_t)0xFFFF)
#define INST_STACK_END      ((address_t)0xC000)

typedef struct _memories
{
    uint8_t *instruction;
    uint8_t *instruction_stack;
    uint8_t *data;
    uint8_t *user_stack;
} memories_t;

#endif // __MEMORY_H__