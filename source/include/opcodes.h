#ifndef __OPCODES_H__
#define __OPCODES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "symbols.h"

#ifdef _MSC_VER 
    //not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strp _stricmp
#endif

#define ERROR_BUFFER_SIZE   1024
#define LINE_BUFFER_SIZE    1024

#define ALU_INST_BASE           0x80
#define FLOW_INST_BASE          0x60
#define OTHER_INST_BASE         0x40
#define STACK_INST_BASE         0x00

// Only applies to ALU and stack instructions
#define OPCODE_SIZE_BIT         (0x20)

// Condition code flags
#define CC_DIV0                 (1 << 5)
#define CC_UNDERFLOW            (1 << 4)
#define CC_ZERO                 (1 << 3)
#define CC_CARRY                (1 << 2)
#define CC_NEG                  (1 << 1)
#define CC_OVERFLOW             (1 << 0)

// ALU instructions
#define OPCODE_ADD                     (ALU_INST_BASE + 0b10000)
#define OPCODE_SUB                     (ALU_INST_BASE + 0b10001)
#define OPCODE_MUL                     (ALU_INST_BASE + 0b00000)
#define OPCODE_DIV                     (ALU_INST_BASE + 0b00001)
#define OPCODE_INC                     (ALU_INST_BASE + 0b11000)
#define OPCODE_DEC                     (ALU_INST_BASE + 0b11001)
#define OPCODE_CMP                     (ALU_INST_BASE + 0b11010)
#define OPCODE_OR                      (ALU_INST_BASE + 0b00010)
#define OPCODE_AND                     (ALU_INST_BASE + 0b00011)
#define OPCODE_SHL                     (ALU_INST_BASE + 0b00110)
#define OPCODE_SHR                     (ALU_INST_BASE + 0b00111)

// Stack instructions
#define OP_STACK_MASK                   0b11111
#define OP_STACK_PUSH                   0b00000
#define OP_STACK_PULL                   0b10000
#define OP_STACK_REGISTER_MASK          0b00011
#define OP_STACK_ONLY                   0b00000
#define OP_STACK_AND_DP                 0b00001
#define OP_STACK_AND_X                  0b00010
#define OP_STACK_REGISTER_INDEXED       0b01000
#define OP_STACK_OTHER                  0b00100
#define OP_STACK_TO_STACK               0b00000
#define OP_STACK_MISC                   0b01000
#define OP_STACK_S                      0b00010
#define OP_STACK_R                      0b00001
#define OP_STACK_COPY                   0b10000
#define OPCODE_PUSHI                   (STACK_INST_BASE + OP_STACK_PUSH)
#define OPCODE_PUSHDP                  (STACK_INST_BASE + OP_STACK_PUSH + OP_STACK_AND_DP)
#define OPCODE_PUSHX                   (STACK_INST_BASE + OP_STACK_PUSH + OP_STACK_AND_X + OPCODE_SIZE_BIT)
#define OPCODE_PUSH_INDEXED            (STACK_INST_BASE + OP_STACK_PUSH + OP_STACK_REGISTER_INDEXED)
#define OPCODE_PUSHDP_INDEXED          (OPCODE_PUSH_INDEXED + OP_STACK_AND_DP)
#define OPCODE_PUSHX_INDEXED           (OPCODE_PUSH_INDEXED + OP_STACK_AND_X)
#define OPCODE_POP                     (STACK_INST_BASE + OP_STACK_PULL)
#define OPCODE_PULLDP                  (STACK_INST_BASE + OP_STACK_PULL + OP_STACK_AND_DP)
#define OPCODE_PULLX                   (STACK_INST_BASE + OP_STACK_PULL + OP_STACK_AND_X + OPCODE_SIZE_BIT)
#define OPCODE_PULL_INDEXED            (STACK_INST_BASE + OP_STACK_PULL + OP_STACK_REGISTER_INDEXED)
#define OPCODE_PULLDP_INDEXED          (OPCODE_PULL_INDEXED + OP_STACK_AND_DP)
#define OPCODE_PULLX_INDEXED           (OPCODE_PULL_INDEXED + OP_STACK_AND_X)
#define OPCODE_MOVER                   (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_TO_STACK + OP_STACK_R)
#define OPCODE_MOVES                   (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_TO_STACK + OP_STACK_S)
#define OPCODE_COPYR                   (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_TO_STACK + OP_STACK_R + OP_STACK_COPY)
#define OPCODE_COPYS                   (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_TO_STACK + OP_STACK_S + OP_STACK_COPY)
#define OPCODE_DUP                     (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_MISC + 0b00001)
#define OPCODE_SWAP                    (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_MISC + 0b00010)
#define OPCODE_DEPTH                   (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_MISC + 0b00011)
#define OPCODE_ROLL                    (STACK_INST_BASE + OP_STACK_OTHER + OP_STACK_MISC + 0b10011)

// Register indexed stack operation bits
// The post-byte for register indexed stack operations has for its
// most-significant byte a flag indicating if the register should be
// incremented before the data is fetched/stored, and the rest
// of the byte is a twos complement amount to apply as the increment
#define OP_STACK_INCREMENT_PRE          0b10000000
#define OP_STACK_INCREMENT_NEGATIVE     0b01000000

// Flow control instructions - immediate address/ID
// TODO: Update to support not-equal, less-than-or-equal, etc
#define OP_FLOW_UNSIGNED                0b10000
#define OP_BRANCH_COMPARE_MASK          0b1111
#define OP_BRANCH_UNCONDITIONAL         0b0000
#define OP_BRANCH_OVERFLOW              0b0001
#define OP_BRANCH_LESS_THAN             0b0010
#define OP_BRANCH_CARRY                 0b0100
#define OP_BRANCH_GREATER               0b0101
#define OP_BRANCH_EQUAL                 0b1000
#define OP_BRANCH_LESS_THAN_EQUAL       0b1010
#define OP_BRANCH_GREATER_EQUAL         0b1101
#define OP_BRANCH_DIVIDE_BY_ZERO        0b1111
#define OPCODE_B                       (FLOW_INST_BASE + OP_BRANCH_UNCONDITIONAL)
#define OPCODE_BEQ                     (FLOW_INST_BASE + OP_BRANCH_EQUAL)
#define OPCODE_BLT                     (FLOW_INST_BASE + OP_BRANCH_LESS_THAN)
#define OPCODE_BLE                     (FLOW_INST_BASE + OP_BRANCH_LESS_THAN_EQUAL)
#define OPCODE_BGT                     (FLOW_INST_BASE + OP_BRANCH_GREATER)
#define OPCODE_BGE                     (FLOW_INST_BASE + OP_BRANCH_GREATER_EQUAL)
#define OPCODE_BCR                     (FLOW_INST_BASE + OP_BRANCH_CARRY)
#define OPCODE_BOV                     (FLOW_INST_BASE + OP_BRANCH_OVERFLOW)
#define OPCODE_BDIV0                   (FLOW_INST_BASE + OP_BRANCH_DIVIDE_BY_ZERO)
#define OPCODE_JMP                     (FLOW_INST_BASE + OP_FLOW_UNSIGNED + 0b0000)
#define OPCODE_RTS                     (FLOW_INST_BASE + OP_FLOW_UNSIGNED + 0b0001)
#define OPCODE_JSR                     (FLOW_INST_BASE + OP_FLOW_UNSIGNED + 0b0010)
#define OPCODE_SYSCALL                 (FLOW_INST_BASE + OP_FLOW_UNSIGNED + 0b1111)

// Other instructions - New arch
#define OPCODE_SYNC                    (OTHER_INST_BASE + 0b11111)

#define IS_STACK_INST(opcode)   ((opcode & 0xC0) == STACK_INST_BASE)
#define IS_INDEXED_INST(opcode) (IS_STACK_INST(opcode) && (opcode & OP_STACK_REGISTER_INDEXED) && !(opcode & OP_STACK_OTHER))
#define IS_BRANCH_INST(opcode)  ((opcode & OP_FLOW_UNSIGNED) == 0 && (opcode & 0xE0) == FLOW_INST_BASE)

typedef enum _register_access_mode
{
    NONE,
    STACK_ONLY,
    IMMEDIATE_OPERANDS,
    STACK_TO_FROM_REGISTER,
    REGISTER_INDEXED,
} register_access_mode_t;

typedef struct opcode_entry
{
    const char *name;
    uint8_t opcode;
    // How many cycles the instruction needs to execute
    // Negative values mean the cycles need to be calculated at runtime
    // 0 means that the emulator has ceded control through a sync or syscall
    int cycles;
    int arg_byte_count;
    symbol_type_t argument_type;
    register_access_mode_t access_mode;
    symbol_signedness_t argument_signedness;
} opcode_entry_t;

typedef struct _register_argument
{
    const char *name;
    uint8_t code;
} register_argument_t;

opcode_entry_t *get_opcode_entry(const char *opcode_name);
opcode_entry_t *get_opcode_entry_from_opcode(uint8_t opcode);
void print_opcode_entries();
register_argument_t *get_register(const char *register_name);

#ifdef __cplusplus
}
#endif

#endif // __OPCODES_H__