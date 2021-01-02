#include "opcodes.h"

#include <string.h>
#include "symbols.h"

opcode_entry_t opcode_entries[] =
{
    // ALU instructions
    { "add",    OPCODE_ADD,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "addw",   OPCODE_ADD + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "sub",    OPCODE_SUB,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "subw",   OPCODE_SUB + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "mul",    OPCODE_MUL,                     6, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "mulw",   OPCODE_MUL + OPCODE_SIZE_BIT,   10, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "div",    OPCODE_DIV,                     6, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "divw",   OPCODE_DIV + OPCODE_SIZE_BIT,   10, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "or",     OPCODE_OR,                      2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "orw",    OPCODE_OR + OPCODE_SIZE_BIT,    4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "and",    OPCODE_AND,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "andw",   OPCODE_AND + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "shr",    OPCODE_SHR,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "shrw",   OPCODE_SHR + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "shl",    OPCODE_SHL,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "shlw",   OPCODE_SHL + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "inc",    OPCODE_INC,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "incw",   OPCODE_INC + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "dec",    OPCODE_DEC,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "decw",   OPCODE_DEC + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "cmp",    OPCODE_CMP,                     2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "cmpw",   OPCODE_CMP + OPCODE_SIZE_BIT,   4, 0, SYMBOL_NO_TYPE,      STACK_ONLY },

    // Stack instructions
    { "pushi",  OPCODE_PUSHI,                   1, 1, SYMBOL_BYTE,         IMMEDIATE_OPERANDS,     SIGNEDNESS_ANY },
    { "pushiw", OPCODE_PUSHI + OPCODE_SIZE_BIT, 2, 2, SYMBOL_WORD,         IMMEDIATE_OPERANDS,     SIGNEDNESS_ANY },
    { "pop",    OPCODE_POP,                     1, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "popw",   OPCODE_POP + OPCODE_SIZE_BIT,   2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "dup",    OPCODE_DUP,                     1, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "dupw",   OPCODE_DUP + OPCODE_SIZE_BIT,   2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "swap",   OPCODE_SWAP,                    1, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "swapw",  OPCODE_SWAP + OPCODE_SIZE_BIT,  2, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "roll",   OPCODE_ROLL,                    -1, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "rollw",  OPCODE_ROLL + OPCODE_SIZE_BIT,  -1, 0, SYMBOL_NO_TYPE,      STACK_ONLY },
    { "pushdp", OPCODE_PUSHDP,                  1, 0, SYMBOL_NO_TYPE,      STACK_TO_FROM_REGISTER },
    { "pushx",  OPCODE_PUSHX,                   2, 0, SYMBOL_NO_TYPE,      STACK_TO_FROM_REGISTER },
    { "pulldp", OPCODE_PULLDP,                  1, 0, SYMBOL_NO_TYPE,      STACK_TO_FROM_REGISTER },
    { "pullx",  OPCODE_PULLX,                   2, 0, SYMBOL_NO_TYPE,      STACK_TO_FROM_REGISTER },
    
    // TODO: The argument byte is a twos complement pre/post increment/decrement
    // Need to find a way to handle these with the individual opcodes
    // Register indexed
    { "push",   OPCODE_PUSH_INDEXED,                    2, 1, SYMBOL_NO_TYPE,  REGISTER_INDEXED },
    { "pushw",  OPCODE_PUSH_INDEXED + OPCODE_SIZE_BIT,  3, 1, SYMBOL_NO_TYPE,  REGISTER_INDEXED },
    { "pull",   OPCODE_PULL_INDEXED,                    2, 1, SYMBOL_NO_TYPE,  REGISTER_INDEXED },
    { "pullw",  OPCODE_PULL_INDEXED + OPCODE_SIZE_BIT,  3, 1, SYMBOL_NO_TYPE,  REGISTER_INDEXED },

    // Flow instructions
    { "b",      OPCODE_B,                       3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "beq",    OPCODE_BEQ,                     3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "blt",    OPCODE_BLT,                     3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "ble",    OPCODE_BLE,                     3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "bcr",    OPCODE_BCR,                     3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "bov",    OPCODE_BOV,                     3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "bdiv0",  OPCODE_BDIV0,                   3, 1, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_SIGNED },
    { "jmp",    OPCODE_JMP,                     4, 2, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_UNSIGNED },
    { "jsr",    OPCODE_JSR,                     5, 2, SYMBOL_ADDRESS_INST, IMMEDIATE_OPERANDS,     SIGNEDNESS_UNSIGNED },
    { "rts",    OPCODE_RTS,                     2, 0, SYMBOL_NO_TYPE,      NONE },
    { "syscall", OPCODE_SYSCALL,                0, 2, SYMBOL_WORD,         IMMEDIATE_OPERANDS,     SIGNEDNESS_ANY },

    { "sync",   OPCODE_SYNC,                    0, 0, SYMBOL_NO_TYPE,      NONE },

    { 0, 0, 0 }
};

register_argument_t registers[] =
{
    { "dp",     OP_STACK_AND_DP },
    { "x",      OP_STACK_AND_X },

    { 0, 0 }
};

opcode_entry_t *get_opcode_entry(const char *opcode_name)
{
    int index = 0;
    while (opcode_entries[index].name != 0)
    {
        if (strncasecmp(opcode_name, opcode_entries[index].name, LINE_BUFFER_SIZE) == 0)
        {
            return &opcode_entries[index];
        }
        index++;
    }

    return 0;
}

int opcode_entry_count()
{
    int count = 0;
    while (opcode_entries[count].name != 0)
    {
        count++;
    }
    return count;
}

opcode_entry_t* get_opcode_entry_by_index(int index)
{
    int count = opcode_entry_count();
    if (index >= count)
    {
        return 0;
    }

    return &opcode_entries[index];
}

opcode_entry_t *get_opcode_entry_from_opcode(uint8_t opcode)
{
    if (IS_INDEXED_INST(opcode))
    {
        opcode &= ~OP_STACK_REGISTER_MASK;
    }

    int index = 0;
    while (opcode_entries[index].name != 0)
    {
        if (opcode == opcode_entries[index].opcode)
        {
            return &opcode_entries[index];
        }
        index++;
    }

    return 0;
}

register_argument_t *get_register(const char *register_name)
{
    int index = 0;
    while (registers[index].name != 0)
    {
        if (strncasecmp(register_name, registers[index].name, LINE_BUFFER_SIZE) == 0)
        {
            return &registers[index];
        }
        index++;
    }

    return 0;
}

int register_count()
{
    int count = 0;
    while (registers[count].name != 0)
    {
        count++;
    }

    return count;
}

register_argument_t* get_register_by_index(int index)
{
    int count = register_count();
    if (index >= count)
    {
        return 0;
    }

    return &registers[index];
}

void print_opcode_entries()
{
    int index = 0;
    while (opcode_entries[index].name != 0)
    {
        printf("Opcode %s has value 0x%02x\n", opcode_entries[index].name, opcode_entries[index].opcode);
        index++;
    }
}