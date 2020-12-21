#include "emulator.h"
#include "opcodes.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

    return reset_emulator(emulator);
}

error_t reset_emulator(emulator *emulator)
{
    memset(emulator->memories.data, 0, DATA_SIZE);
    memset(emulator->memories.instruction_stack, 0, INST_STACK_SIZE);
    memset(emulator->memories.user_stack, 0, STACK_SIZE);

    emulator->current_state = RUNNING;
    emulator->graphics_mode.enabled = 0;
    emulator->graphics_start = 0;
    emulator->PC = EXECUTE_BEGIN;
    emulator->SP = 0;
    emulator->ISP = 0;
    emulator->CC = 0;
    emulator->X = 0;
    emulator->DP = 0;

    return NO_ERROR;
}

uint16_t fetch_instruction_word(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.data[emulator->PC];
    value.bytes[0] = emulator->memories.data[emulator->PC+1];
    emulator->PC += 2;
    return value.word;
}

int16_t fetch_instruction_word_signed(emulator *emulator)
{
    emulator_word_t value;
    value.bytes[1] = emulator->memories.data[emulator->PC];
    value.bytes[0] = emulator->memories.data[emulator->PC+1];
    emulator->PC += 2;
    return *((int16_t*)&value);
}

uint8_t fetch_instruction_byte(emulator *emulator)
{
    uint8_t byte = emulator->memories.data[emulator->PC];
    emulator->PC++;
    return byte;
}

uint8_t peek_instruction_byte(emulator *emulator)
{
    return emulator->memories.data[emulator->PC];
}

int8_t fetch_instruction_byte_signed(emulator *emulator)
{
    int8_t byte = ((int8_t*)emulator->memories.data)[emulator->PC];
    emulator->PC++;
    return byte;
}

uint16_t pull_word(emulator *emulator)
{
    emulator->SP -= 2;
    emulator_word_t value;
    value.bytes[0] = emulator->memories.user_stack[emulator->SP + 1];
    value.bytes[1] = emulator->memories.user_stack[emulator->SP];
    return value.word;
}

int16_t pull_word_signed(emulator *emulator)
{
    emulator->SP -= 2;
    emulator_word_t value;
    value.bytes[0] = emulator->memories.user_stack[emulator->SP + 1];
    value.bytes[1] = emulator->memories.user_stack[emulator->SP];
    return *((int16_t*)&value);
}

uint8_t pull_byte(emulator *emulator)
{
    emulator->SP--;
    uint8_t value = emulator->memories.user_stack[emulator->SP];
    return value;
}

int8_t pull_byte_signed(emulator *emulator)
{
    emulator->SP--;
    int8_t value = ((int8_t*)emulator->memories.user_stack)[emulator->SP];
    return value;
}

uint16_t peek_word(emulator *emulator, uint8_t index)
{
    emulator_word_t value;
    value.bytes[0] = emulator->memories.user_stack[emulator->SP - index - 1];
    value.bytes[1] = emulator->memories.user_stack[emulator->SP - index - 2];
    return value.word;
}

uint8_t peek_byte(emulator *emulator, uint8_t index)
{
    uint8_t value = emulator->memories.user_stack[emulator->SP - index - 1];
    return value;
}

void push_word(emulator *emulator, uint16_t word)
{
    emulator_word_t emWord;
    emWord.word = word;
    emulator->memories.user_stack[emulator->SP + 1] = emWord.bytes[0];
    emulator->memories.user_stack[emulator->SP] = emWord.bytes[1];
    emulator->SP += 2;
}

void push_byte(emulator *emulator, uint8_t byte)
{
    emulator->memories.user_stack[emulator->SP] = byte;
    emulator->SP++;
}

void pop_bytes(emulator *emulator, uint16_t byte_count)
{
    emulator->SP -= byte_count;
}

uint16_t pull_return_address(emulator *emulator)
{
    emulator->ISP -= 2;
    emulator_word_t value;
    value.bytes[0] = emulator->memories.instruction_stack[emulator->ISP + 1];
    value.bytes[1] = emulator->memories.instruction_stack[emulator->ISP];
    return value.word;
}

void push_return_address(emulator *emulator, uint16_t address)
{
    emulator_word_t emWord;
    emWord.word = address;
    emulator->memories.instruction_stack[emulator->ISP + 1] = emWord.bytes[0];
    emulator->memories.instruction_stack[emulator->ISP] = emWord.bytes[1];
    emulator->ISP += 2;
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

void set_data_indexed_byte(emulator *emulator, uint8_t byte, uint16_t index)
{
    emulator->memories.data[index] = byte;
}

uint8_t get_data_indexed_byte(emulator *emulator, uint16_t index)
{
    return emulator->memories.data[index];
}

void set_data_indexed_word(emulator* emulator, uint16_t word, uint16_t index)
{
    emulator_word_t emWord;
    emWord.word = word;
    emulator->memories.data[index + 1] = emWord.bytes[0];
    emulator->memories.data[index] = emWord.bytes[1];
}

// get_stack_indexed_word would be the same as peek_word with an index,
// so an index parameter was just added to peek_word
#define get_stack_indexed_word(emulator, index) peek_word(emulator, index)
#define get_stack_indexed_byte(emulator, index) peek_byte(emulator, index)

uint16_t get_data_indexed_word(emulator* emulator, uint16_t index)
{
    emulator_word_t value;
    value.bytes[0] = emulator->memories.data[index + 1];
    value.bytes[1] = emulator->memories.data[index];
    return value.word;
}

execute_result_t execute_alu_instruction(emulator *emulator, uint8_t opcode)
{
    execute_result_t result;
    result.status = EXEC_NORMAL;

    uint8_t wide = opcode & OPCODE_SIZE_BIT;
    uint8_t baseopcode = opcode & ~OPCODE_SIZE_BIT;

    emulator->CC = 0;

    // TODO:
    // - Find compiler-independent carry/overflow check methods
    if (baseopcode == OPCODE_INC || baseopcode == OPCODE_DEC)
    {
        if (wide)
        {
            int16_t operand = pull_word_signed(emulator);
            int16_t op_result = (baseopcode == OPCODE_INC) ? (operand + 1) : (operand - 1);
            push_word(emulator, op_result);
        }
        else
        {
            int8_t operand = pull_byte_signed(emulator);
            int8_t op_result = (baseopcode == OPCODE_INC) ? (operand + 1) : (operand - 1);
            push_byte(emulator, op_result);
        }
     }
    else if (wide)
    {
        int16_t operandB = pull_word_signed(emulator);
        int16_t operandA = pull_word_signed(emulator);
        int16_t op_result = 0;

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

            case OPCODE_DIV:
                op_result = operandA / operandB;
                if (operandB == 0)
                {
                    emulator->CC |= CC_DIV0;
                }
                break;

            case OPCODE_CMP:
                if (operandA == operandB)
                {
                    emulator->CC |= CC_ZERO;
                }
                else
                {
                    emulator->CC &= ~CC_ZERO;
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
                    emulator->CC |= CC_CARRY;
                }
                break;

            case OPCODE_SHR:
                op_result = operandA >> operandB;
                if (operandA & 1)
                {
                    emulator->CC |= CC_UNDERFLOW;
                }
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }

        if (result.status != EXEC_ILLEGAL_INSTRUCTION && baseopcode != OPCODE_CMP)
        {
            uint16_t uword = *((uint16_t*)&op_result);
            push_word(emulator, uword);
            if (uword == 0)
            {
                emulator->CC |= CC_ZERO;
            }
            else if (uword & 0x8000)
            {
                emulator->CC |= CC_NEG;
            }
        }
    }
    else
    {
        int8_t operandB = pull_byte_signed(emulator);
        int8_t operandA = pull_byte_signed(emulator);
        int8_t op_result = 0;

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
                emulator->CC |= CC_ZERO;
            }
            else
            {
                emulator->CC &= ~CC_ZERO;
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
                emulator->CC |= CC_CARRY;
            }
            else
            {
                emulator->CC &= ~CC_CARRY;
            }
            break;

        case OPCODE_SHR:
            op_result = operandA >> operandB;
            break;

        default:
            result.status = EXEC_ILLEGAL_INSTRUCTION;
            break;
        }

        if (result.status != EXEC_ILLEGAL_INSTRUCTION && baseopcode != OPCODE_CMP)
        {
            uint8_t ubyte = *((uint8_t*)&op_result);
            push_byte(emulator, ubyte);
            if (ubyte == 0)
            {
                emulator->CC |= CC_ZERO;
                emulator->CC &= ~CC_NEG;
            }
            else if (ubyte & 0x80)
            {
                emulator->CC &= ~CC_ZERO;
                emulator->CC |= CC_NEG;
            }
        }
    }

    return result;
}

void execute_dup(emulator *emulator, uint8_t is_wide)
{
    if (is_wide)
    {
        uint16_t word = peek_word(emulator, 0);
        push_word(emulator, word);
    }
    else
    {
        uint8_t byte = peek_byte(emulator, 0);
        push_byte(emulator, byte);
    }
}

void execute_swap(emulator *emulator, uint8_t is_wide)
{
    if (is_wide)
    {
        uint16_t word1 = pull_word(emulator);
        uint16_t word2 = pull_word(emulator);
        push_word(emulator, word1);
        push_word(emulator, word2);
    }
    else
    {
        uint8_t byte1 = pull_byte(emulator);
        uint8_t byte2 = pull_byte(emulator);
        push_byte(emulator, byte1);
        push_byte(emulator, byte2);
    }
}

execute_status_t execute_roll(emulator *emulator, uint8_t is_wide)
{
    uint8_t count = pull_byte(emulator);
    uint8_t byte_count = is_wide ? count * 2 : count;
    if (byte_count > emulator->SP)
    {
        return EXEC_ILLEGAL_INSTRUCTION;
    }

    uint8_t *rolled_address = &emulator->memories.user_stack[emulator->SP - byte_count];
    if (is_wide)
    {
        uint16_t *word_rolled_address = (uint16_t*)rolled_address;
        machine_word_t rolled_word;
        rolled_word.uword = *(word_rolled_address);
        for (uint8_t place = 0; place < count - 1; place++)
        {
            word_rolled_address[place] = word_rolled_address[place+1];
        }
        emulator->memories.user_stack[emulator->SP - 1] = rolled_word.bytes[0];
        emulator->memories.user_stack[emulator->SP - 2] = rolled_word.bytes[1];
    }
    else
    {
        uint8_t rolled_byte = *rolled_address;
        for (uint8_t place = 0; place < count - 1; place++)
        {
            rolled_address[place] = rolled_address[place+1];
        }
        emulator->memories.user_stack[emulator->SP - 1] = rolled_byte;
    }

    return EXEC_NORMAL;
}

execute_result_t execute_stack_instruction(emulator *emulator, uint8_t opcode)
{
    execute_result_t result;
    result.status = EXEC_NORMAL;

    uint8_t is_wide = opcode & OPCODE_SIZE_BIT;
    uint8_t is_other = opcode & OP_STACK_OTHER;
    uint8_t baseopcode = opcode & ~OPCODE_SIZE_BIT;

    if (is_other)
    {
        uint8_t is_misc = opcode & OP_STACK_MISC;
        if (is_misc)
        {
            switch (baseopcode)
            {
            case OPCODE_DUP:
                execute_dup(emulator, is_wide);
                break;

            case OPCODE_SWAP:
                execute_swap(emulator, is_wide);
                break;

            case OPCODE_ROLL:
                result.status = execute_roll(emulator, is_wide);
                break;

            case OPCODE_DEPTH:
                push_byte(emulator, emulator->SP);
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }
        else
        {
            // TODO: This needs an implementation
            uint8_t is_copy = opcode & OP_STACK_COPY;
            uint8_t register_argument = opcode & OP_STACK_REGISTER_MASK;
            switch (register_argument)
            {
            case OP_STACK_S:
                break;

            case OP_STACK_R:
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }
    }
    else
    {
        uint8_t register_argument = baseopcode & OP_STACK_REGISTER_MASK;
        uint8_t is_pull = baseopcode & OP_STACK_PULL;
        uint8_t is_indexed = IS_INDEXED_INST(baseopcode);
        if (is_indexed)
        {
            uint16_t index;
            uint8_t increment = fetch_instruction_byte(emulator);
            uint8_t pre_increment = increment & OP_STACK_INCREMENT_PRE;
            uint8_t decrement = increment & OP_STACK_INCREMENT_NEGATIVE;
            // Replace the top bit with the negative bit to get a twos complement number
            // aka mask out the pre-increment bit and sign-extend the remaining byte
            int8_t increment_signed = ((increment & 0b01111111) | (decrement << 1));

            switch (register_argument)
            {
            case OP_STACK_AND_DP:
                if (pre_increment)
                {
                    emulator->DP += increment_signed;
                }
                index = emulator->DP;
               break;
            
            case OP_STACK_AND_X:
                if (pre_increment)
                {
                    emulator->X += increment_signed;
                }
                index = emulator->X;
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }

            if (result.status != EXEC_ILLEGAL_INSTRUCTION)
            {
                if (is_pull)
                {
                    if (is_wide)
                    {
                        set_data_indexed_word(emulator, pull_word(emulator), index);
                    }
                    else
                    {
                        set_data_indexed_byte(emulator, pull_byte(emulator), index);
                    }
                }
                else
                {
                    if (is_wide)
                    {
                        push_word(emulator, get_data_indexed_word(emulator, index));
                    }
                    else
                    {
                        push_byte(emulator, get_data_indexed_byte(emulator, index));
                    }
                }

                if (!pre_increment)
                {
                    switch (register_argument)
                    {
                    case OP_STACK_AND_DP:
                        emulator->DP += increment_signed;
                        break;
                    
                    case OP_STACK_AND_X:
                        emulator->X += increment_signed;
                        break;
                    }
                }
            }
        }
        else if (is_pull)
        {
            switch (register_argument)
            {
            case OP_STACK_ONLY:
                pop_bytes(emulator, is_wide ? 2 : 1);
                break;
            
            case OP_STACK_AND_DP:
                if (is_wide)
                {
                    result.status = EXEC_ILLEGAL_INSTRUCTION;
                }
                else
                {
                    emulator->DP = pull_byte(emulator);
                }
                break;

            case OP_STACK_AND_X:
                if (is_wide)
                {
                    emulator->X = pull_word(emulator);
                }
                else
                {
                    result.status = EXEC_ILLEGAL_INSTRUCTION;
                }
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }
        else // !is_indexed && !is_pull, aka push
        {
            uint16_t word;
            uint8_t byte;
            switch (register_argument)
            {
            case OP_STACK_ONLY:
                if (is_wide)
                {
                    word = fetch_instruction_word(emulator);
                    push_word(emulator, word);
                }
                else
                {
                    byte = fetch_instruction_byte(emulator);
                    push_byte(emulator, byte);
                }
                break;

            case OP_STACK_AND_DP:
                if (is_wide)
                {
                    result.status = EXEC_ILLEGAL_INSTRUCTION;
                }
                else
                {
                    push_byte(emulator, emulator->DP);
                }
                break;

            case OP_STACK_AND_X:
                if (is_wide)
                {
                    push_word(emulator, emulator->X);
                }
                else
                {
                    result.status = EXEC_ILLEGAL_INSTRUCTION;
                }
                break;

            default:
                result.status = EXEC_ILLEGAL_INSTRUCTION;
                break;
            }
        }
    }

    return result;
}

void get_debug_info(emulator *emulator, char *debugging_buffers[DEBUGGING_BUFFER_COUNT])
{
    address_t original_pc = emulator->PC;
    uint8_t opcode = peek_instruction_byte(emulator);
    inst_result_t result = SUCCESS;
    address_t new_pc = original_pc + 2;
    uint8_t imm_msb = emulator->memories.data[original_pc + 1];
    uint8_t imm_lsb = emulator->memories.data[original_pc + 2];
    address_t imm_16 = ((uint16_t)imm_msb << 8) | imm_lsb;
    address_t twos_new_pc = original_pc + (((int16_t)imm_msb << 8) | (int16_t)imm_lsb);
    
    if (debugging_buffers != 0)
    {
        int current_buffer = 0;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < LINE_BUFFER_SIZE; j++)
            {
                debugging_buffers[i][j] = 0;
            }
        }
        const char *state_string = "Unknown exec state";
        switch (emulator->current_state)
        {
        case RUNNING:
            state_string = "Running";
            break;

        case FINISHED:
            state_string = "Finished";
            break;

        case WAITING:
            state_string = "Waiting";
            break;

        case ERROR:
            state_string = "error";
            break;

        case DEBUGGING:
            state_string = "Debugging";
            break;
        }
        int length = snprintf(debugging_buffers[current_buffer++], LINE_BUFFER_SIZE, "Machine state: PC: 0x%04x, SP: 0x%02x, ISP: 0x%02x,",
            emulator->PC,
            emulator->SP,
            emulator->ISP
        );
        
        length = snprintf(debugging_buffers[current_buffer++], LINE_BUFFER_SIZE, "X: 0x%04x, CC: 0x%02x, DP: 0x%02x (%s)",
            emulator->X,
            emulator->CC,
            emulator->DP,
            state_string
        );

        int index = 0;
        index += snprintf(&debugging_buffers[current_buffer][index], LINE_BUFFER_SIZE, "Stack: ");
        if (emulator->SP > 0)
        {
            size_t remaining = LINE_BUFFER_SIZE - index;
            for (int i = emulator->SP - 1; i >= 0 && remaining > 8; --i)
            {
                index += snprintf(&debugging_buffers[current_buffer][index], remaining, "0x%02x ", emulator->memories.user_stack[i]);
                remaining = LINE_BUFFER_SIZE - index;
            }
        }
        debugging_buffers[current_buffer++][index] = 0;

        snprintf(debugging_buffers[current_buffer++], LINE_BUFFER_SIZE, "[DP] 0x%02x [X] 0x%02x", emulator->memories.data[emulator->DP], emulator->memories.data[emulator->X]);

        opcode_entry_t *current_opcode_entry = get_opcode_entry_from_opcode(opcode);
        if (current_opcode_entry != 0)
        {
            if (IS_INDEXED_INST(opcode))
            {
                const char *prefix = "";
                if (imm_msb & OP_STACK_INCREMENT_PRE)
                {
                    prefix = "pre-";
                }

                if (imm_msb & OP_STACK_INCREMENT_NEGATIVE)
                {
                    imm_msb |= OP_STACK_INCREMENT_PRE;
                }
                else
                {
                    imm_msb &= ~OP_STACK_INCREMENT_PRE;
                }
                
                const char *register_name = "DP";
                if ((opcode & OP_STACK_REGISTER_MASK) == OP_STACK_AND_X)
                {
                    register_name = "X";
                }

                int8_t increment = *((int8_t*)&imm_msb);
                length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Opcode: %s, %sincrementing register %s by %d", current_opcode_entry->name, prefix, register_name, increment);
            }
            else if (current_opcode_entry->arg_byte_count == 1)
            {
                length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Opcode: %s, post-byte: 0x%02x", current_opcode_entry->name, imm_msb);
            }
            else if (current_opcode_entry->argument_type == SYMBOL_ADDRESS_INST)
            {
                if (current_opcode_entry->argument_signedness == SIGNEDNESS_SIGNED)
                {
                    machine_word_t word;
                    word.bytes[0] = imm_lsb;
                    word.bytes[1] = imm_msb;
                    length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Branch: %s, by %d", current_opcode_entry->name, word.sword);
                }
                else
                {
                    length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Opcode: %s, operand 0x%04x\n", current_opcode_entry->name, imm_16);
                }    
            }
            else if (current_opcode_entry->argument_type == SYMBOL_WORD)
            {
                length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Opcode: %s, operand 0x%04x\n", current_opcode_entry->name, imm_16);
            }
            else
            {
                length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Opcode: %s\n", current_opcode_entry->name);
            }
        }
        else
        {
            length = snprintf(debugging_buffers[current_buffer], LINE_BUFFER_SIZE, "Failed to find opcode %02x\n", opcode);
        }
        debugging_buffers[current_buffer][length] = 0;
    }
}

inst_result_t execute_instruction(emulator *emulator, opcode_entry_t** executed_instruction)
{
    address_t original_pc = emulator->PC;
    uint8_t opcode = fetch_instruction_byte(emulator);
    inst_result_t result = SUCCESS;
    address_t new_pc = original_pc + 2;
    uint8_t imm_msb = emulator->memories.data[original_pc + 1];
    uint8_t imm_lsb = emulator->memories.data[original_pc + 2];
    int8_t imm_signed = *((int8_t*)&imm_msb);
    address_t imm_16 = ((uint16_t)imm_msb << 8) | imm_lsb;
    address_t twos_new_pc = original_pc + imm_signed;

    *executed_instruction = get_opcode_entry_from_opcode(opcode);
 
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
        uint8_t branch = 0;
        switch (opcode)
        {        
        case OPCODE_B:
            branch = 1;
            break;
        
        case OPCODE_BEQ:
            if (emulator->CC & CC_ZERO)
            {
                branch = 1;
            }
            break;

        case OPCODE_BCR:
            if (emulator->CC & CC_CARRY)
            {
                branch = 1;
            }
            break;

        case OPCODE_BLT:
            if (emulator->CC & CC_NEG)
            {
                branch = 1;
            }
            break;

        case OPCODE_BOV:
            if (emulator->CC & CC_OVERFLOW)
            {
                branch = 1;
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
        
        case OPCODE_SYNC:
            new_pc--;
            result = SYNC;
            break;

        default:
            result = ILLEGAL_INSTRUCTION; 
            break;
        }
    
        if (branch)
        {
            new_pc = twos_new_pc;
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
