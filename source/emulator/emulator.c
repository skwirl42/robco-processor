#include "emulator.h"
#include "opcodes.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef union
{
    uint16_t word;
    uint8_t bytes[2];
} emulator_word_t;

error_t dispose_emulator(emulator *emulator);

error_t init_emulator(emulator *emulator, arch_t architecture)
{
    if (emulator == 0)
    {
        return ARG_NULL;
    }

    if (architecture != ARCH_ORIGINAL)
    {
        return ARCH_UNSUPPORTED;
    }

    emulator->architecture = architecture;

    emulator->memories.instruction = malloc(INST_SIZE);

    if (emulator->memories.instruction == 0)
    {
        dispose_emulator(emulator);
        return ALLOC_FAILED;
    }

    emulator->memories.data = malloc(DATA_SIZE);

    if (emulator->memories.data == 0)
    {
        dispose_emulator(emulator);
        return ALLOC_FAILED;
    }

    emulator->memories.instruction_stack = malloc(INST_STACK_SIZE);

    if (emulator->memories.instruction_stack == 0)
    {
        dispose_emulator(emulator);
        return ALLOC_FAILED;        
    }

    emulator->memories.user_stack = malloc(STACK_SIZE);

    if (emulator->memories.user_stack == 0)
    {
        dispose_emulator(emulator);
        return ALLOC_FAILED;
    }

    memset(emulator->memories.instruction, 0, INST_SIZE);
    memset(emulator->memories.data, 0, DATA_SIZE);
    memset(emulator->memories.instruction_stack, 0, INST_STACK_SIZE);
    memset(emulator->memories.user_stack, 0, STACK_SIZE);

    emulator->current_state = RUNNING;
    emulator->PC = EXECUTE_BEGIN;
    emulator->SP = STACK_BEGIN;
    emulator->ISP = INST_STACK_BEGIN;
    emulator->CC = 0;
    emulator->X = 0;
    emulator->SI = 0;
    emulator->DP = 0;

    return NO_ERROR;
}

uint16_t fetch_instruction_word(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.instruction[emulator->PC];
    value.bytes[0] = emulator->memories.instruction[emulator->PC+1];
    emulator->PC += 2;
    return value.word;
}

int16_t fetch_instruction_word_signed(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.instruction[emulator->PC];
    value.bytes[0] = emulator->memories.instruction[emulator->PC+1];
    emulator->PC += 2;
    return *((int16_t*)&value);
}

uint8_t fetch_instruction_byte(emulator *emulator)
{
    uint8_t byte = emulator->memories.instruction[emulator->PC];
    emulator->PC++;
    return byte;
}

int8_t fetch_instruction_byte_signed(emulator *emulator)
{
    int8_t byte = ((int8_t*)emulator->memories.instruction)[emulator->PC];
    emulator->PC++;
    return byte;
}

uint16_t pull_word(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.user_stack[emulator->SP - STACK_END];
    value.bytes[0] = emulator->memories.user_stack[emulator->SP+1 - STACK_END];
    emulator->SP += 2;
    return value.word;
}

int16_t pull_word_signed(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.user_stack[emulator->SP - STACK_END];
    value.bytes[0] = emulator->memories.user_stack[emulator->SP+1 - STACK_END];
    emulator->SP += 2;
    return *((int16_t*)&value);
}

uint8_t pull_byte(emulator *emulator)
{
    uint8_t value = emulator->memories.user_stack[emulator->SP - STACK_END];
    emulator->SP++;
    return value;
}

int8_t pull_byte_signed(emulator *emulator)
{
    int8_t value = ((int8_t*)emulator->memories.user_stack)[emulator->SP - STACK_END];
    emulator->SP++;
    return value;
}

uint16_t peek_word(emulator *emulator, uint8_t index)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.user_stack[emulator->SP + index - STACK_END];
    value.bytes[0] = emulator->memories.user_stack[emulator->SP+1 + index - STACK_END];
    return value.word;
}

uint8_t peek_byte(emulator *emulator, uint8_t index)
{
    uint8_t value = emulator->memories.user_stack[emulator->SP + index - STACK_END];
    return value;
}

void push_word(emulator *emulator, uint16_t word)
{
    emulator->SP -= 2;
    emulator_word_t emWord;
    emWord.word = word;
    emulator->memories.user_stack[emulator->SP - STACK_END] = emWord.bytes[1];
    emulator->memories.user_stack[emulator->SP + 1 - STACK_END] = emWord.bytes[0];
}

void push_byte(emulator *emulator, uint8_t byte)
{
    emulator->SP--;
    emulator->memories.user_stack[emulator->SP - STACK_END] = byte;
}

void pop_bytes(emulator *emulator, uint16_t byte_count)
{
    emulator->SP += byte_count;
}

uint16_t pull_return_address(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.instruction_stack[emulator->ISP - INST_STACK_END];
    value.bytes[0] = emulator->memories.instruction_stack[emulator->ISP+1 - INST_STACK_END];
    emulator->ISP += 2;
    return value.word;
}

void push_return_address(emulator *emulator, uint16_t address)
{
    emulator->ISP -= 2;
    emulator_word_t emWord;
    emWord.word = address;
    emulator->memories.instruction_stack[emulator->ISP - INST_STACK_END] = emWord.bytes[1];
    emulator->memories.instruction_stack[emulator->ISP + 1 - INST_STACK_END] = emWord.bytes[0];
}

typedef enum
{
    EXEC_NORMAL,
    EXEC_BRANCH,
    EXEC_JUMP,
    EXEC_ILLEGAL_INSTRUCTION,
} execute_status_t;

typedef struct _execute_result_t
{
    execute_status_t status;
    uint16_t absolute;
    int16_t relative;
} execute_result_t;

void set_stack_indexed_word(emulator* emulator, uint16_t word)
{
    emulator_word_t emWord;
    emWord.word = word;
    emulator->memories.user_stack[emulator->SP + emulator->SI - STACK_END] = emWord.bytes[1];
    emulator->memories.user_stack[emulator->SP + emulator->SI + 1 - STACK_END] = emWord.bytes[0];
}

void set_direct_page_word(emulator* emulator, uint16_t word)
{
    emulator_word_t emWord;
    emWord.word = word;
    emulator->memories.data[emulator->DP] = emWord.bytes[1];
    emulator->memories.data[emulator->DP] = emWord.bytes[0];
}

// get_stack_indexed_word would be the same as peek_word with an index,
// so an index parameter was just added to peek_word

uint16_t get_direct_page_word(emulator* emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.data[emulator->DP - STACK_END];
    value.bytes[0] = emulator->memories.data[emulator->DP + 1 - STACK_END];
    return value.word;
}

execute_status_t get_byte_for_operand(emulator* emulator, uint8_t operand, uint8_t *result)
{
    switch (operand)
    {
    case STACK_OPERAND:
         *result = pull_byte(emulator);
         return EXEC_NORMAL;

    case STACK_INDEX:
        *result = emulator->memories.user_stack[emulator->SP + emulator->SI - STACK_END];
        return EXEC_NORMAL;

    case DIRECT_PAGE:
        *result = emulator->memories.data[emulator->DP];
        return EXEC_NORMAL;

    default:
        return EXEC_ILLEGAL_INSTRUCTION;
    }
}

execute_status_t set_byte_for_operand(emulator* emulator, uint8_t operand, uint8_t value)
{
    switch (operand)
    {
    case STACK_OPERAND:
        push_byte(emulator, value);
        return EXEC_NORMAL;

    case STACK_INDEX:
        emulator->memories.user_stack[emulator->SP + emulator->SI - STACK_END] = value;
        return EXEC_NORMAL;

    case DIRECT_PAGE:
        emulator->memories.data[emulator->DP] = value;
        return EXEC_NORMAL;

    default:
        return EXEC_ILLEGAL_INSTRUCTION;
    }
}

execute_status_t get_word_for_operand(emulator* emulator, uint8_t operand, uint16_t* result)
{
    switch (operand)
    {
    case STACK_OPERAND:
        *result = pull_word(emulator);
        return EXEC_NORMAL;

    case STACK_INDEX:
        *result = peek_word(emulator, emulator->SI);
        return EXEC_NORMAL;

    case DIRECT_PAGE:
        *result = get_direct_page_word(emulator);
        return EXEC_NORMAL;

    default:
        return EXEC_ILLEGAL_INSTRUCTION;
    }
}

execute_status_t set_word_for_operand(emulator* emulator, uint8_t operand, uint16_t value)
{
    switch (operand)
    {
    case STACK_OPERAND:
        push_word(emulator, value);
        return EXEC_NORMAL;

    case STACK_INDEX:
        set_stack_indexed_word(emulator, value);
        return EXEC_NORMAL;

    case DIRECT_PAGE:
        set_direct_page_word(emulator, value);
        return EXEC_NORMAL;

    default:
        return EXEC_ILLEGAL_INSTRUCTION;
    }
}

execute_result_t execute_alu_instruction(emulator *emulator, uint8_t opcode)
{
    execute_result_t result;
    result.status = EXEC_NORMAL;

    uint8_t wide = opcode & OPCODE_SIZE_BIT;
    uint8_t baseopcode = opcode & ~OPCODE_SIZE_BIT;
    uint8_t sources = fetch_instruction_byte(emulator);
    uint8_t source0 = SOURCE_0(sources);
    uint8_t source1 = SOURCE_1(sources);
    uint8_t source2 = SOURCE_2(sources);
    uint8_t destination = DESTINATION(sources);

    // TODO:
    // - Find compiler-independent carry/overflow check methods
    if (baseopcode == OPCODE_INC || baseopcode == OPCODE_DEC)
    {
        if (wide)
        {
            int16_t operand;
            execute_status_t get_result = get_word_for_operand(emulator, destination, (uint16_t*)&operand);
            if (get_result != EXEC_ILLEGAL_INSTRUCTION)
            {
                int16_t op_result = (baseopcode == OPCODE_INC) ? (operand + 1) : (operand - 1);
                result.status = set_word_for_operand(emulator, destination, *((uint16_t*)&op_result));
            }
            else
            {
                result.status = get_result;
            }
        }
        else
        {
            int8_t operand;
            execute_status_t get_result = get_byte_for_operand(emulator, destination, (uint8_t*)&operand);
            if (get_result != EXEC_ILLEGAL_INSTRUCTION)
            {
                int8_t op_result = (baseopcode == OPCODE_INC) ? (operand + 1) : (operand - 1);
                result.status = set_byte_for_operand(emulator, destination, *((uint8_t*)&op_result));
            }
            else
            {
                result.status = get_result;
            }
        }
     }
    else if (wide)
    {
        int16_t operandB = 0;
        int16_t operandA = 0;
        int16_t op_result = 0;

        result.status = get_word_for_operand(emulator, source1, (uint16_t*)&operandB);
        if (result.status != EXEC_ILLEGAL_INSTRUCTION)
        {
            result.status = get_word_for_operand(emulator, source0, (uint16_t*)&operandA);
        }

        if (result.status != EXEC_ILLEGAL_INSTRUCTION)
        {
            switch (baseopcode)
            {
            case OPCODE_ADD:
                op_result = operandA + operandB;
                break;

            case OPCODE_SUB:
                op_result = operandA - operandB;
                break;

            case OPCODE_MUL:
                op_result = operandA * operandB;
                break;

            case OPCODE_CMP:
                if (operandA == operandB)
                {
                    emulator->CC |= CC_Z;
                }
                break;

            case OPCODE_OR:
                op_result = operandA | operandB;
                break;

            case OPCODE_AND:
                op_result = operandA & operandB;
                break;

            case OPCODE_SHL:
                op_result = operandA << operandB;
                if (operandA & 0x8000)
                {
                    emulator->CC |= CC_C;
                }
                break;

            case OPCODE_SHR:
                op_result = operandA >> operandB;
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }

        if (result.status != EXEC_ILLEGAL_INSTRUCTION && baseopcode != OPCODE_CMP)
        {
            uint16_t uword = *((uint16_t*)&op_result);
            result.status = set_word_for_operand(emulator, destination, uword);
            if (uword == 0)
            {
                emulator->CC |= CC_Z;
            }
            else if (uword & 0x8000)
            {
                emulator->CC |= CC_N;
            }
        }
    }
    else
    {
        int8_t operandB = 0;
        int8_t operandA = 0;
        int8_t op_result = 0;

        result.status = get_byte_for_operand(emulator, source1, (uint8_t*)&operandB);
        if (result.status != EXEC_ILLEGAL_INSTRUCTION)
        {
            result.status = get_byte_for_operand(emulator, source0, (uint8_t*)&operandA);
        }

        if (result.status != EXEC_ILLEGAL_INSTRUCTION)
        {
            switch (baseopcode)
            {
            case OPCODE_ADD:
                op_result = operandA + operandB;
                break;

            case OPCODE_SUB:
                op_result = operandA - operandB;
                break;

            case OPCODE_MUL:
                op_result = operandA * operandB;
                break;

            case OPCODE_CMP:
                if (operandA == operandB)
                {
                    emulator->CC |= CC_Z;
                }
                break;

            case OPCODE_OR:
                op_result = operandA | operandB;
                break;

            case OPCODE_AND:
                op_result = operandA & operandB;
                break;

            case OPCODE_SHL:
                op_result = operandA << operandB;
                if (operandA & 0x80)
                {
                    emulator->CC |= CC_C;
                }
                break;

            case OPCODE_SHR:
                op_result = operandA >> operandB;
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }

        if (result.status != EXEC_ILLEGAL_INSTRUCTION && baseopcode != OPCODE_CMP)
        {
            uint8_t ubyte = *((uint8_t*)&op_result);
            result.status = set_byte_for_operand(emulator, destination, ubyte);
            if (ubyte == 0)
            {
                emulator->CC |= CC_Z;
            }
            else if (ubyte & 0x80)
            {
                emulator->CC |= CC_N;
            }
        }
    }

    return result;
}

execute_result_t execute_stack_instruction(emulator *emulator, uint8_t opcode)
{
    execute_result_t result;
    result.status = EXEC_NORMAL;

    uint8_t wide = opcode & OPCODE_SIZE_BIT;
    uint8_t baseopcode = opcode & ~OPCODE_SIZE_BIT;
    uint8_t sources = 0;

    // PUSHI has its value set by the two bytes following the opcode.
    // Anything else uses the second byte as sources
    if (baseopcode != OPCODE_PUSHI)
    {
        sources = fetch_instruction_byte(emulator);
    }

    if (baseopcode == OPCODE_PULL)
    {
        uint8_t destination = DESTINATION(sources);
        if (wide && destination == X_REGISTER)
        {
            emulator->X = pull_word(emulator);
        }
        else if (!wide && (destination == STACK_INDEX || destination == DIRECT_PAGE))
        {
            switch (destination)
            {
            case STACK_INDEX:
                emulator->SI = pull_byte(emulator);
                break;

            case DIRECT_PAGE:
                emulator->DP = pull_byte(emulator);
                break;
            }
        }
        else
        {
            result.status = EXEC_ILLEGAL_INSTRUCTION;
        }
    }
    else if (baseopcode == OPCODE_PUSH)
    {
        uint8_t source = SOURCE_0(sources);
        if (wide && source == X_REGISTER)
        {
            push_word(emulator, emulator->X);
        }
        else if (!wide && (source == STACK_INDEX || source == DIRECT_PAGE))
        {
            switch (source)
            {
            case STACK_INDEX:
                push_byte(emulator, emulator->SI);
                break;

            case DIRECT_PAGE:
                push_byte(emulator, emulator->DP);
                break;
            }
        }
        else
        {
            result.status = EXEC_ILLEGAL_INSTRUCTION;
        }
    }
    else if (wide)
    {
        uint16_t word0;
        uint16_t word1;
        uint8_t byte;
        uint16_t roll_byte_depth;
        emulator_word_t roll_word;
        switch (baseopcode)
        {
        case OPCODE_PUSHI:
            word0 = fetch_instruction_word(emulator);
            push_word(emulator, word0);
            break;

        case OPCODE_POP:
            pop_bytes(emulator, 2);
            break;
            
        case OPCODE_DUP:
            word0 = peek_word(emulator, 0);
            push_word(emulator, word0);
            break;
            
        case OPCODE_SWAP:
            word1 = pull_word(emulator);
            word0 = pull_word(emulator);
            push_word(emulator, word1);
            push_word(emulator, word0);
            break;
            
        case OPCODE_ROLL:
            byte = pull_byte(emulator);
            roll_byte_depth = byte * 2;
            roll_word.bytes[1] = emulator->memories.user_stack[emulator->SP + roll_byte_depth - STACK_END];
            roll_word.bytes[0] = emulator->memories.user_stack[emulator->SP + roll_byte_depth + 1 - STACK_END];
            for (int currentByte = roll_byte_depth + 1; currentByte > 1; currentByte--)
            {
                emulator->memories.user_stack[emulator->SP + currentByte - STACK_END] = emulator->memories.user_stack[emulator->SP + currentByte - 2 - STACK_END];
            }
            emulator->memories.user_stack[emulator->SP - STACK_END] = roll_word.bytes[1];
            emulator->memories.user_stack[emulator->SP + 1 - STACK_END] = roll_word.bytes[0];
            break;
            
        default:
            result.status = EXEC_ILLEGAL_INSTRUCTION;
            break;
        }
    }
    else
    {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t roll_depth;
        switch (baseopcode)
        {
        case OPCODE_PUSHI:
            byte0 = fetch_instruction_byte(emulator);
            push_byte(emulator, byte0);
            break;

        case OPCODE_POP:
            pop_bytes(emulator, 1);
            break;
            
        case OPCODE_DUP:
            byte0 = peek_byte(emulator, 0);
            push_byte(emulator, byte0);
            break;
            
        case OPCODE_SWAP:
            byte1 = pull_byte(emulator);
            byte0 = pull_byte(emulator);
            push_byte(emulator, byte1);
            push_byte(emulator, byte0);
            break;
            
        case OPCODE_ROLL:
            roll_depth = pull_byte(emulator);
            byte0 = emulator->memories.user_stack[emulator->SP + roll_depth - STACK_END];
            for (int currentByte = roll_depth; currentByte > 0; currentByte--)
            {
                emulator->memories.user_stack[emulator->SP + currentByte - STACK_END] = emulator->memories.user_stack[emulator->SP + currentByte - 1 - STACK_END];
            }
            emulator->memories.user_stack[emulator->SP - STACK_END] = byte0;
            break;
            
        default:
            result.status = EXEC_ILLEGAL_INSTRUCTION;
            break;
        }
    }

    return result;
}

inst_result_t execute_instruction(emulator *emulator)
{
    address_t original_pc = emulator->PC;
    uint8_t opcode = fetch_instruction_byte(emulator);
    inst_result_t result = SUCCESS;
    address_t new_pc = original_pc + 2;
    uint8_t imm_msb = emulator->memories.instruction[original_pc + 1];
    uint8_t imm_lsb = emulator->memories.instruction[original_pc + 2];
    address_t imm_16 = ((uint16_t)imm_msb << 8) | imm_lsb;
    address_t twos_new_pc = original_pc + (((int16_t)imm_msb << 8) | (int16_t)imm_lsb);
    
    if (opcode & ALU_INST_BASE)
    {
        execute_result_t execresult = execute_alu_instruction(emulator, opcode);
        if (execresult.status == EXEC_ILLEGAL_INSTRUCTION)
        {
            result = ILLEGAL_INSTRUCTION;
        }
    }
    else if (IS_STACK_INST(opcode))
    {
        execute_result_t execresult = execute_stack_instruction(emulator, opcode);
        if (execresult.status == EXEC_ILLEGAL_INSTRUCTION)
        {
            result = ILLEGAL_INSTRUCTION;
        }
    }
    else
    {
        switch (opcode)
        {        
        case OPCODE_B:
            new_pc = twos_new_pc;
            break;
        
        case OPCODE_BE:
        case OPCODE_BC:
        case OPCODE_BN:
        case OPCODE_BO:
            new_pc++;
            if (emulator->CC & opcode)
            {
                new_pc = twos_new_pc;
            }
            break;
        
        case OPCODE_JMP:
            new_pc = imm_16;
            break;

        case OPCODE_JSR:
            new_pc = imm_16;
            push_return_address(emulator, original_pc + 3);
            break;

        case OPCODE_RTS:
            new_pc = pull_return_address(emulator);
            break;
        
        case OPCODE_SYSCALL:
            new_pc++;
            emulator->current_syscall = imm_16;
            result = EXECUTE_SYSCALL;
            break;
        
        default:
            result = ILLEGAL_INSTRUCTION; 
            break;
        }
    
        emulator->PC = new_pc;
    }

    if (result == ILLEGAL_INSTRUCTION)
    {
        emulator->current_state = ERROR;
    }

    return result;
}

uint8_t emulator_can_execute(emulator *emulator)
{
    return emulator->current_state == RUNNING;
}

error_t dispose_emulator(emulator *emulator)
{
    if (emulator->memories.instruction != 0)
    {
        free(emulator->memories.instruction);
    }

    if (emulator->memories.instruction_stack != 0)
    {
        free(emulator->memories.instruction_stack);
    }

    if (emulator->memories.data != 0)
    {
        free(emulator->memories.data);
    }

    if (emulator->memories.user_stack != 0)
    {
        free(emulator->memories.user_stack);
    }

    emulator->architecture = ARCH_NONE;

    return NO_ERROR;
}
