#ifndef __OPCODES_H__
#define __OPCODES_H__

#define ALU_INST_BASE           0x80
#define FLOW_INST_BASE          0x60
#define OTHER_INST_BASE         0x40
#define STACK_INST_BASE         0x00

// Only applies to ALU and stack instructions
#define SIZE_BIT                0x20

// Condition code flags
#define CC_Z                    (1 << 3)
#define CC_C                    (1 << 2)
#define CC_N                    (1 << 1)
#define CC_O                    (1 << 0)

// ALU instructions
#define ADD                     (ALU_INST_BASE + 0b10000)
#define SUB                     (ALU_INST_BASE + 0b10001)
#define MUL                     (ALU_INST_BASE + 0b00000)
#define DIV                     (ALU_INST_BASE + 0b00001)
#define MULA                    (ALU_INST_BASE + 0b00010)
#define INC                     (ALU_INST_BASE + 0b11000)
#define DEC                     (ALU_INST_BASE + 0b11001)
#define CMP                     (ALU_INST_BASE + 0b11010)
#define OR                      (ALU_INST_BASE + 0b00010)
#define AND                     (ALU_INST_BASE + 0b00011)
#define ROL                     (ALU_INST_BASE + 0b00100)
#define ROR                     (ALU_INST_BASE + 0b00101)
#define SHL                     (ALU_INST_BASE + 0b00110)
#define SHR                     (ALU_INST_BASE + 0b00111)

// Stack instructions
#define PUSHI                   (STACK_INST_BASE + 0b00000)
#define POP                     (STACK_INST_BASE + 0b01000)
#define DUP                     (STACK_INST_BASE + 0b00100)
#define SWAP                    (STACK_INST_BASE + 0b00010)
#define ROLL                    (STACK_INST_BASE + 0b11111)
// New architecture - size determined by register targeted
#define PUSH                    (STACK_INST_BASE + 0b10000) // Source 0 indicates register to push
#define PULL                    (STACK_INST_BASE + 0b01000) // Destination indicates register to pull the value into

// Flow control instructions - immediate address/ID
#define B                       (FLOW_INST_BASE + 0b00000)
#define BE                      (FLOW_INST_BASE + 0b01000)
#define BC                      (FLOW_INST_BASE + 0b00100)
#define BN                      (FLOW_INST_BASE + 0b00010)
#define BO                      (FLOW_INST_BASE + 0b00001)
#define JMP                     (FLOW_INST_BASE + 0b10000)
#define SYSCALL                 (FLOW_INST_BASE + 0b11111)

// Flow control instructions - New architecture, X register based
#define JMPX                    (FLOW_INST_BASE + 0b10001)
#define JSR                     (FLOW_INST_BASE + 0b10010)

// Other instructions - New arch
#define SYNC                    (OTHER_INST_BASE + 0b11111)

#define SOURCE_0_MASK           0b11000000
#define SOURCE_1_MASK           0b00110000
#define SOURCE_2_MASK           0b00001100
#define DESTINATION_MASK        0b00000011

#endif // __OPCODES_H__