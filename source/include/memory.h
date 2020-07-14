#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

typedef uint16_t address_t;

#define MEMSIZE         ((user_addr_t)0x100000000)
#define STACKBEGIN      ((address_t)0xFFFFFFFF)
#define EXECUTEBEGIN    ((address_t)0x00000000)

typedef struct _memories
{
    uint8_t *instruction;
    uint8_t *data;
} memories_t;

#endif // __MEMORY_H__