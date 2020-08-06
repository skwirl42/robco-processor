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
#define strcasecmp _stricmp
#endif

#define ERROR_BUFFER_SIZE   1024
#define LINE_BUFFER_SIZE    1024

#define ALU_INST_BASE           0x80
#define FLOW_INST_BASE          0x60
#define OTHER_INST_BASE         0x40
#define STACK_INST_BASE         0x00

#define IS_STACK_INST(opcode)   ((opcode & 0xC0) == STACK_INST_BASE)
#define IS_BRANCH_INST(opcode)  ((opcode & 0b10000) == 0 && (opcode & 0xE0) == FLOW_INST_BASE)

// Only applies to ALU and stack instructions
#define OPCODE_SIZE_BIT         (0x20)

// Condition code flags
#define CC_Z                    (1 << 3)
#define CC_C                    (1 << 2)
#define CC_N                    (1 << 1)
#define CC_O                    (1 << 0)

// ALU instructions
#define OPCODE_ADD                     (ALU_INST_BASE + 0b10000)
#define OPCODE_SUB                     (ALU_INST_BASE + 0b10001)
#define OPCODE_MUL                     (ALU_INST_BASE + 0b00000)
// #define OPCODE_DIV                     (ALU_INST_BASE + 0b00001)
// #define OPCODE_MULA                    (ALU_INST_BASE + 0b00010) // definitely invalid... need to change when implementing new ISA
#define OPCODE_INC                     (ALU_INST_BASE + 0b11000)
#define OPCODE_DEC                     (ALU_INST_BASE + 0b11001)
#define OPCODE_CMP                     (ALU_INST_BASE + 0b11010)
#define OPCODE_OR                      (ALU_INST_BASE + 0b00010)
#define OPCODE_AND                     (ALU_INST_BASE + 0b00011)
#define OPCODE_SHL                     (ALU_INST_BASE + 0b00110)
#define OPCODE_SHR                     (ALU_INST_BASE + 0b00111)

// Stack instructions
#define OPCODE_PUSHI                   (STACK_INST_BASE + 0b00000)
#define OPCODE_POP                     (STACK_INST_BASE + 0b01000)
#define OPCODE_DUP                     (STACK_INST_BASE + 0b00100)
#define OPCODE_SWAP                    (STACK_INST_BASE + 0b00010)
#define OPCODE_ROLL                    (STACK_INST_BASE + 0b11111)
#define OPCODE_PUSH                    (STACK_INST_BASE + 0b10000) // Source 0 indicates register to push
#define OPCODE_PULL                    (STACK_INST_BASE + 0b01001) // Destination indicates register to pull the value into
#define OPCODE_PUSH_INDEXED            (STACK_INST_BASE + 0b10010)
#define OPCODE_PULL_INDEXED            (STACK_INST_BASE + 0b01010)

// Flow control instructions - immediate address/ID
#define OPCODE_B                       (FLOW_INST_BASE + 0b00000)
#define OPCODE_BE                      (FLOW_INST_BASE + CC_Z)
#define OPCODE_BC                      (FLOW_INST_BASE + CC_C)
#define OPCODE_BN                      (FLOW_INST_BASE + CC_N)
#define OPCODE_BO                      (FLOW_INST_BASE + CC_O)
#define OPCODE_JMP                     (FLOW_INST_BASE + 0b10000)
#define OPCODE_RTS                     (FLOW_INST_BASE + 0b10001)
#define OPCODE_JSR                     (FLOW_INST_BASE + 0b10010)
#define OPCODE_SYSCALL                 (FLOW_INST_BASE + 0b11111)

// Other instructions - New arch
#define OPCODE_SYNC                    (OTHER_INST_BASE + 0b11111)

#define SOURCE_0_MASK           0b11000000
#define SOURCE_0(BYTE)			((BYTE & SOURCE_0_MASK) >> 6)
#define TO_SOURCE_0(BYTE)       (BYTE << 6)
#define SOURCE_1_MASK           0b00110000
#define SOURCE_1(BYTE)			((BYTE & SOURCE_1_MASK) >> 4)
#define TO_SOURCE_1(BYTE)       (BYTE << 4)
#define SOURCE_2_MASK           0b00001100
#define SOURCE_2(BYTE)			((BYTE & SOURCE_2_MASK) >> 2)
#define TO_SOURCE_2(BYTE)       (BYTE << 2)
#define DESTINATION_MASK        0b00000011
#define DESTINATION(BYTE)		(BYTE & DESTINATION_MASK)
#define TO_DESTINATION(BYTE)    (BYTE)

#define STACK_OPERAND			0b00
#define STACK_INDEX				0b01
#define DIRECT_PAGE				0b10
#define X_REGISTER				0b11

typedef enum _register_access_mode
{
    NONE,
    STACK_ONLY,
    IMMEDIATE_OPERANDS,
    STACK_TO_FROM_REGISTER,
    REGISTER_INDEXED,
} register_access_mode_t;

typedef union
{
    uint16_t uword;
    int16_t sword;
    uint8_t bytes[2];
} machine_word_t;

typedef struct opcode_entry
{
    const char *name;
    uint8_t opcode;
    int arg_byte_count;
    symbol_type_t argument_type;
    register_access_mode_t access_mode;
    symbol_signedness_t argument_signedness;
    uint8_t use_specified_operand;
    uint8_t operands;
} opcode_entry_t;

typedef struct _register_argument
{
    const char *name;
    uint8_t code;
} register_argument_t;

opcode_entry_t *get_opcode_entry(const char *opcode_name);
opcode_entry_t *get_opcode_entry_from_opcode(uint8_t opcode);
register_argument_t *get_register(const char *name);
register_argument_t *get_register_by_code(uint8_t code);

#ifdef __cplusplus
}
#endif

#endif // __OPCODES_H__