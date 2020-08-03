#include "opcodes.h"

#include <string.h>
#include "symbols.h"

opcode_entry_t opcode_entries[] =
{
    // ALU instructions
    { "add",    OPCODE_ADD,                     1, SYMBOL_NO_TYPE },
    { "addw",   OPCODE_ADD + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "sub",    OPCODE_SUB,                     1, SYMBOL_NO_TYPE },
    { "subw",   OPCODE_SUB + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "mul",    OPCODE_MUL,                     1, SYMBOL_NO_TYPE },
    { "mulw",   OPCODE_MUL + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "or",     OPCODE_OR,                      1, SYMBOL_NO_TYPE },
    { "orw",    OPCODE_OR + OPCODE_SIZE_BIT,    1, SYMBOL_NO_TYPE },
    { "and",    OPCODE_AND,                     1, SYMBOL_NO_TYPE },
    { "andw",   OPCODE_AND + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "shr",    OPCODE_SHR,                     1, SYMBOL_NO_TYPE },
    { "shrw",   OPCODE_SHR + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "shl",    OPCODE_SHL,                     1, SYMBOL_NO_TYPE },
    { "shlw",   OPCODE_SHL + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "inc",    OPCODE_INC,                     1, SYMBOL_NO_TYPE },
    { "incw",   OPCODE_INC + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "dec",    OPCODE_DEC,                     1, SYMBOL_NO_TYPE },
    { "decw",   OPCODE_DEC + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "cmp",    OPCODE_CMP,                     1, SYMBOL_NO_TYPE },
    { "cmpw",   OPCODE_CMP + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },

    // Stack instructions
    { "pushi",  OPCODE_PUSHI,                   1, SYMBOL_BYTE,         SIGNEDNESS_ANY },
    { "pushiw", OPCODE_PUSHI + OPCODE_SIZE_BIT, 2, SYMBOL_WORD,         SIGNEDNESS_ANY },
    { "pop",    OPCODE_POP,                     1, SYMBOL_NO_TYPE },
    { "popw",   OPCODE_POP + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "dup",    OPCODE_DUP,                     1, SYMBOL_NO_TYPE },
    { "dupw",   OPCODE_DUP + OPCODE_SIZE_BIT,   1, SYMBOL_NO_TYPE },
    { "swap",   OPCODE_SWAP,                    1, SYMBOL_NO_TYPE },
    { "swapw",  OPCODE_SWAP + OPCODE_SIZE_BIT,  1, SYMBOL_NO_TYPE },
    { "roll",   OPCODE_ROLL,                    1, SYMBOL_NO_TYPE },
    { "rollw",  OPCODE_ROLL + OPCODE_SIZE_BIT,  1, SYMBOL_NO_TYPE },
    { "pushsi", OPCODE_PUSH,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         1, (STACK_INDEX << 6) },
    { "pushdp", OPCODE_PUSH,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         1, (DIRECT_PAGE << 6) },
    { "pushx",  OPCODE_PUSH + OPCODE_SIZE_BIT,  1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         1, (X_REGISTER << 6) },
    { "pullsi", OPCODE_PULL,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         1, STACK_INDEX },
    { "pulldp", OPCODE_PULL,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         1, DIRECT_PAGE },
    { "pullx",  OPCODE_PULL + OPCODE_SIZE_BIT,  1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         1, X_REGISTER },

    // Flow instructions
    { "b",      OPCODE_B,                       2, SYMBOL_ADDRESS_INST, SIGNEDNESS_SIGNED },
    { "be",     OPCODE_BE,                      2, SYMBOL_ADDRESS_INST, SIGNEDNESS_SIGNED },
    { "bn",     OPCODE_BN,                      2, SYMBOL_ADDRESS_INST, SIGNEDNESS_SIGNED },
    { "bc",     OPCODE_BC,                      2, SYMBOL_ADDRESS_INST, SIGNEDNESS_SIGNED },
    { "bo",     OPCODE_BO,                      2, SYMBOL_ADDRESS_INST, SIGNEDNESS_SIGNED },
    { "jmp",    OPCODE_JMP,                     2, SYMBOL_ADDRESS_INST, SIGNEDNESS_UNSIGNED },
    { "jsr",    OPCODE_JSR,                     2, SYMBOL_ADDRESS_INST, SIGNEDNESS_UNSIGNED },
    { "rts",    OPCODE_RTS,                     1, SYMBOL_NO_TYPE },
    { "syscall", OPCODE_SYSCALL,                2, SYMBOL_WORD,         SIGNEDNESS_ANY },

    { 0, 0, 0 }
};

register_argument_t registers[] =
{
    { "so",     STACK_OPERAND },
    { "si",     STACK_INDEX },
    { "dp",     DIRECT_PAGE },
    { "x",      X_REGISTER },

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

opcode_entry_t *get_opcode_entry_from_opcode(uint8_t opcode)
{
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

register_argument_t *get_register(const char *name)
{
    int index = 0;
    while (registers[index].name != 0)
    {
        if (strncasecmp(name, registers[index].name, LINE_BUFFER_SIZE) == 0)
        {
            return &registers[index];
        }
        index++;
    }

    return 0;
}

register_argument_t *get_register_by_code(uint8_t code)
{
    int index = 0;
    while (registers[index].name != 0)
    {
        if (registers[index].code == code)
        {
            return &registers[index];
        }
        index++;
    }

    return 0;
}
