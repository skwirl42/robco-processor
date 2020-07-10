#include "emulator.h"
#include "opcodes.h"

#include <string.h>

error_t init_emulator(emulator *emulator, arch_t architecture)
{
    if (emulator == USER_ADDR_NULL)
    {
        return ARG_NULL;
    }

    if (architecture != ARCH_ORIGINAL)
    {
        return ARCH_UNSUPPORTED;
    }

    emulator->architecture = architecture;

    emulator->memories.instruction = malloc(MEMSIZE);

    if (emulator->memories.instruction == USER_ADDR_NULL)
    {
        return ALLOC_FAILED;
    }

    emulator->memories.data = malloc(MEMSIZE);

    if (emulator->memories.data == USER_ADDR_NULL)
    {
        free(emulator->memories.instruction);
        return ALLOC_FAILED;
    }

    memset(emulator->memories.instruction, 0, MEMSIZE);
    memset(emulator->memories.data, 0, MEMSIZE);

    emulator->PC = EXECUTEBEGIN;
    emulator->SP = STACKBEGIN;
    emulator->SR = 0;
    emulator->CC = 0;

    if (emulator->architecture == ARCH_NEW)
    {
        emulator->X = 0;
        emulator->SI = 0;
        emulator->DP = 0;
    }

    return NO_ERROR;
}

inst_result_t execute_instruction(emulator *emulator)
{
    uint8_t opcode = emulator->memories.instruction[emulator->PC];
    inst_result_t result = SUCCESS;
    address_t original_pc = emulator->PC;
    address_t new_pc = original_pc + 2;
    uint8_t imm_msb = emulator->memories.instruction[original_pc + 1];
    uint8_t imm_lsb = emulator->memories.instruction[original_pc + 2];
    address_t imm_16 = ((uint16_t)imm_msb << 8) | imm_lsb;
    address_t twos_new_pc = original_pc + (((int16_t)imm_msb << 8) | (int16_t)imm_lsb);
    uint8_t *stack_8 = &emulator->memories.data[emulator->SP];
    uint16_t *stack_16 = ((uint16_t)&emulator->memories.data[emulator->SP]);
    int8_t *twos_8_stack = ((int8_t)&emulator->memories.data[emulator->SP]);
    int16_t *twos_16_stack = ((int8_t)&emulator->memories.data[emulator->SP]);
    int stack_change = 0;
    uint8_t stack_value_msb_valid = 0;
    uint8_t stack_value_lsb_valid = 0;
    uint8_t stack_value_msb = 0;
    uint8_t stack_value_lsb = 0;
    uint8_t wide = opcode & SIZE_BIT;
    uint8_t temp8;
    uint16_t temp16;
    int8_t acc8;
    uint8_t acc8_valid = 0;
    int16_t acc16;
    uint8_t acc16_valid = 0;
    switch (opcode)
    {
        case ADD:
            stack_change = 1;
            acc8 = twos_8_stack[0] + twos_8_stack[1];
            acc8_valid = 1;
            // TODO: CC calculations
            break;

        case ADD + SIZE_BIT:
            stack_change = 2;
            acc16 = twos_16_stack[0] + twos_16_stack[1];
            acc16_valid = 1;
            // TODO: CC calculations
            break;

        case SUB:
            stack_change = 1;
            acc8 = twos_8_stack[1] - twos_8_stack[0];
            acc8_valid = 1;
            // TODO: CC calculations
            break;

        case SUB + SIZE_BIT:
            stack_change = 2;
            acc16 = twos_16_stack[1] - twos_16_stack[0];
            acc16_valid = 1;
            // TODO: CC calculations
            break;
            
        case MUL:
            stack_change = 1;
            acc8 = twos_8_stack[1] * twos_8_stack[0];
            acc8_valid = 1;
            // TODO: CC
            break;

        case MUL + SIZE_BIT:
            stack_change = 2;
            acc16 = twos_16_stack[1] * twos_16_stack[0];
            acc16_valid = 1;
            // TODO: CC
            break;
            
        case INC:
            acc8 = twos_8_stack[0] + 1;
            acc8_valid = 1;
            // TODO: CC
            break;

        case INC + SIZE_BIT:
            acc16 = twos_16_stack[0] + 1;
            acc16_valid = 1;
            // TODO: CC
            break;
            
        case DEC:
            acc8 = twos_8_stack[0] - 1;
            acc8_valid = 1;
            // TODO: CC
            break;

        case DEC + SIZE_BIT:
            acc16 = twos_16_stack[0] - 1;
            acc16_valid = 1;
            // TODO: CC
            break;
            
        case OR:
            stack_change = 1;
            acc8 = twos_8_stack[1] | twos_8_stack[0];
            acc8_valid = 1;
            // TODO: CC
            break;

        case OR + SIZE_BIT:
            stack_change = 2;
            acc16 = twos_16_stack[1] | twos_16_stack[0];
            acc16_valid = 1;
            // TODO: CC
            break;
            
        case AND:
            stack_change = 1;
            acc8 = twos_8_stack[1] & twos_8_stack[0];
            acc8_valid = 1;
            // TODO: CC
            break;

        case AND + SIZE_BIT:
            stack_change = 2;
            acc16 = twos_16_stack[1] & twos_16_stack[0];
            acc16_valid = 1;
            // TODO: CC
            break;
            
        case CMP:
            stack_change = 2;
            emulator->CC |= (twos_8_stack[1] == twos_8_stack[0]) ? CC_Z : 0;
            break;

        case CMP + SIZE_BIT:
            stack_change = 4;
            emulator->CC |= (twos_16_stack[1] == twos_16_stack[0]) ? CC_Z : 0;
            break;
            
        case ROL:
            stack_change = 1;
            // TODO
            break;

        case ROL + SIZE_BIT:
            stack_change = 2;
            // TODO
            break;
            
        case ROR:
            stack_change = 1;
            // TODO
            break;

        case ROR + SIZE_BIT:
            stack_change = 2;
            // TODO
            break;
            
        case SHL:
            stack_change = 1;
            // TODO
            break;

        case SHL + SIZE_BIT:
            stack_change = 2;
            // TODO
            break;
            
        case SHR:
            stack_change = 1;
            // TODO
            break;

        case SHR + SIZE_BIT:
            stack_change = 2;
            // TODO
            break;
        
        case PUSHI:
            stack_change = -1;
            stack_value_msb = imm_msb;
            stack_value_msb_valid = 1;
            break;

        case PUSHI + SIZE_BIT:
            stack_change = -2;
            stack_value_msb = imm_msb;
            stack_value_msb_valid = 1;
            stack_value_lsb = imm_lsb;
            stack_value_lsb_valid = 1;
            break;
        
        case POP:
            stack_change = 1;
            break;

        case POP + SIZE_BIT:
            stack_change = 2;
            break;
        
        case ROLL:
            stack_change = 1;
            // TODO
            break;

        case ROLL + SIZE_BIT:
            stack_change = 2;
            // TODO
            break;
        
        case SWAP:
            temp8 = stack_8[0];
            stack_8[0] = stack_8[1];
            stack_8[1] = temp8;
            break;

        case SWAP + SIZE_BIT:
            temp16 = stack_16[0];
            stack_16[0] = stack_16[1];
            stack_16[1] = temp16;
            break;
        
        case DUP:
            stack_change = -1;
            stack_value_msb = emulator->memories.data[emulator->SP];
            stack_value_msb_valid = 1;
        case DUP + SIZE_BIT:
            stack_change--;
            stack_value_lsb = emulator->memories.data[emulator->SP + 1];
            stack_value_lsb_valid = 1;
            break;
        
        case B:
            new_pc = twos_new_pc;
            break;
        
        case BE:
        case BC:
        case BN:
        case BO:
            new_pc++;
            if (emulator->CC & opcode)
            {
                new_pc = twos_new_pc;
            }
            break;
        
        case JMP:
            new_pc = imm_16;
            break;
        
        case SYSCALL:
            emulator->SR = ++new_pc;
            emulator->current_syscall = imm_16;
            result = SYSCALL;
            break;
        
        default:
            result = ILLEGAL_INSTRUCTION; 
            break;
    }

    emulator->PC = new_pc;
    emulator->SP += stack_change;

    if (stack_value_msb_valid)
    {
        emulator->memories.data[emulator->SP] = stack_value_msb;
    }

    if (stack_value_lsb_valid)
    {
        emulator->memories.data[emulator->SP + 1] = stack_value_lsb;
    }

    return result;
}

error_t dispose_emulator(emulator *emulator)
{
    if (emulator->memories.instruction != USER_ADDR_NULL)
    {
        free(emulator->memories.instruction);
    }

    if (emulator->memories.data != USER_ADDR_NULL)
    {
        free(emulator->memories.data);
    }

    emulator->architecture = ARCH_NONE;

    return NO_ERROR;
}
