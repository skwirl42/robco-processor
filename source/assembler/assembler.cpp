#include "assembler_internal.h"

#include "opcodes.h"
#include "memory.h"

opcode_entry_t opcode_entries[] =
{
    // ALU instructions
    { "add", OPCODE_ADD, 1 },
    { "addw", OPCODE_ADD + OPCODE_SIZE_BIT, 1 },
    { "sub", OPCODE_SUB, 1 },
    { "subw", OPCODE_SUB + OPCODE_SIZE_BIT, 1 },
    { "mul", OPCODE_MUL, 1 },
    { "mulw", OPCODE_MUL + OPCODE_SIZE_BIT, 1 },
    { "or", OPCODE_OR, 1 },
    { "orw", OPCODE_OR + OPCODE_SIZE_BIT, 1 },
    { "and", OPCODE_AND, 1 },
    { "andw", OPCODE_AND + OPCODE_SIZE_BIT, 1 },
    { "shr", OPCODE_SHR, 1 },
    { "shrw", OPCODE_SHR + OPCODE_SIZE_BIT, 1 },
    { "shl", OPCODE_SHL, 1 },
    { "shlw", OPCODE_SHL + OPCODE_SIZE_BIT, 1 },
    { "inc", OPCODE_INC, 1 },
    { "incw", OPCODE_INC + OPCODE_SIZE_BIT, 1 },
    { "dec", OPCODE_DEC, 1 },
    { "decw", OPCODE_DEC + OPCODE_SIZE_BIT, 1 },
    { "cmp", OPCODE_CMP, 1 },
    { "cmpw", OPCODE_CMP + OPCODE_SIZE_BIT, 1 },

    // Stack instructions
    { "pushi", OPCODE_PUSHI, 1, SYMBOL_BYTE },
    { "pushiw", OPCODE_PUSHI + OPCODE_SIZE_BIT, 2, SYMBOL_WORD },
    { "pop", OPCODE_POP, 1 },
    { "popw", OPCODE_POP + OPCODE_SIZE_BIT, 1 },
    { "dup", OPCODE_DUP, 1 },
    { "dupw", OPCODE_DUP + OPCODE_SIZE_BIT, 1 },
    { "swap", OPCODE_SWAP, 1 },
    { "swapw", OPCODE_SWAP + OPCODE_SIZE_BIT, 1 },
    { "roll", OPCODE_ROLL, 1 },
    { "rollw", OPCODE_ROLL + OPCODE_SIZE_BIT, 1 },
    { "pushsi", OPCODE_PUSH, 1, SYMBOL_NO_TYPE, true, (STACK_INDEX << 6) },
    { "pushdp", OPCODE_PUSH, 1, SYMBOL_NO_TYPE, true, (DIRECT_PAGE << 6) },
    { "pushx", OPCODE_PUSH + OPCODE_SIZE_BIT, 1, SYMBOL_NO_TYPE, true, (X_REGISTER << 6) },
    { "pullsi", OPCODE_PULL, 1, SYMBOL_NO_TYPE, true, STACK_INDEX },
    { "pulldp", OPCODE_PULL, 1, SYMBOL_NO_TYPE, true, DIRECT_PAGE },
    { "pullx", OPCODE_PULL + OPCODE_SIZE_BIT, 1, SYMBOL_NO_TYPE, true, X_REGISTER },

    // Flow instructions
    { "b", OPCODE_B, 2, SYMBOL_ADDRESS_INST },
    { "be", OPCODE_BE, 2, SYMBOL_ADDRESS_INST },
    { "bn", OPCODE_BN, 2, SYMBOL_ADDRESS_INST },
    { "bc", OPCODE_BC, 2, SYMBOL_ADDRESS_INST },
    { "bo", OPCODE_BO, 2, SYMBOL_ADDRESS_INST },
    { "jmp", OPCODE_JMP, 2, SYMBOL_ADDRESS_INST },
    { "jsr", OPCODE_JSR, 2, SYMBOL_ADDRESS_INST },
    { "rts", OPCODE_RTS, 1, SYMBOL_NO_TYPE },
    { "syscall", OPCODE_SYSCALL, 2, SYMBOL_WORD },

    { 0, 0, 0 }
};

static char error_buffer[ERROR_BUFFER_SIZE + 1];
assembler_data_t *assembler_data;

assembler_result_t handle_include(assembler_data_t *data, const char *included_file)
{
    fprintf(stdout, "Including %s\n", included_file);

    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    return result;
}

assembler_result_t handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;

    symbol_error_t sym_err;
    switch (type)
    {
    case SYMBOL_WORD:
    case SYMBOL_ADDRESS_INST:
    case SYMBOL_ADDRESS_DATA:
        sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, value, 0);
        break;

    case SYMBOL_BYTE:
        sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, 0, value);
        break;
    }

    if (sym_err == SYMBOL_ERROR_NOERROR)
    {
        result.status = ASSEMBLER_SUCCESS;
    }
    else if (sym_err == SYMBOL_ERROR_ALLOC_FAILED)
    {
        snprintf(error_buffer, ERROR_BUFFER_SIZE, "Couldn't allocate space for a symbol named %s", name);
        result.status = ASSEMBLER_ALLOC_FAILED;
    }
    else if (sym_err == SYMBOL_ERROR_EXISTS)
    {
        snprintf(error_buffer, ERROR_BUFFER_SIZE, "Tried to redefine a symbol named %s", name);
        result.status = ASSEMBLER_SYMBOL_ERROR;
    }

    return result;
}

opcode_entry_t *get_opcode_entry(const char *opcode_name)
{
    int index = 0;
    opcode_entry_t *result;
    while (opcode_entries[index].name != 0)
    {
        if (strncasecmp(opcode_name, opcode_entries[index].name, strlen(opcode_entries[index].name)) == 0)
        {
            return &opcode_entries[index];
        }
        index++;
    }

    return 0;
}

assembler_result_t handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;

    fprintf(stdout, "Handling instruction %s with arg (%s)\n", opcode->name, symbol_arg ? symbol_arg : "numerical");
    if (symbol_arg)
    {
        symbol_type_t type;
        symbol_signedness_t signedness;
        uint16_t word_value;
        uint8_t byte_value;
        auto resolution = resolve_symbol(data->symbol_table, symbol_arg, &type, &signedness, &word_value, &byte_value);
        if (resolution == SYMBOL_ASSIGNED)
        {
            switch (type)
            {
            case SYMBOL_WORD:
                fprintf(stdout, "Got value %d for symbol %s\n", word_value, symbol_arg);
                break;

            case SYMBOL_BYTE:
                fprintf(stdout, "Got value %d for symbol %s\n", byte_value, symbol_arg);
                break;

            case SYMBOL_ADDRESS_INST:
                fprintf(stdout, "Got value %04x for instruction address symbol %s\n", word_value, symbol_arg);
                break;

            case SYMBOL_ADDRESS_DATA:
                fprintf(stdout, "Got value %04x for data address symbol %s\n", word_value, symbol_arg);
                break;
            }
        }
        else
        {
            fprintf(stderr, "Could not resolve symbol %s\n", symbol_arg);
        }
    }
    else
    {
        // handle literal
    }

    return result;
}

assembler_result_t assemble(const char *filename, const char **search_paths)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;
    assembler_data_t data;
    data.search_paths = search_paths;
    data.data = new uint8_t[DATA_SIZE];
    data.data_size = 0;
    data.instruction = new uint8_t[INST_SIZE];
    data.instruction_size = 0;
    data.lineNumber = 0;
    assembler_data = &data;

    if (result.status != ASSEMBLER_INTERNAL_ERROR)
    {
        if (create_symbol_table(&data.symbol_table) != SYMBOL_TABLE_NOERROR)
        {
            result.error = "Failed to create the symbol table";
            result.status = ASSEMBLER_ALLOC_FAILED;
        }

        if (result.status == ASSEMBLER_NOOUTPUT)
        {
            FILE *file = fopen(filename, "r");
            if (file != 0)
            {
                int lineNumber = 1;
                char lineBuffer[LINE_BUFFER_SIZE + 1];
                int charIndex = 0;
                int currentChar;
                while ((currentChar = fgetc(file)) != EOF)
                {
                    if (currentChar == '\r' || currentChar == '\n' || currentChar == EOF)
                    {
                        data.lineNumber = lineNumber;
                        lineBuffer[charIndex] = 0;
                        // Process the line
                        if (charIndex > 0)
                        {
                            parse_line(lineBuffer);
                        }

                        charIndex = 0;
                        lineNumber++;
                        continue;
                    }

                    if (charIndex >= LINE_BUFFER_SIZE)
                    {
                        // Deal with this case
                        snprintf(error_buffer, ERROR_BUFFER_SIZE, "Line %d is too long", lineNumber);
                        result.error = error_buffer;
                        result.status = ASSEMBLER_INPUT_ERROR;
                    }
                    else
                    {
                        lineBuffer[charIndex++] = currentChar;
                    }
                }

                if (currentChar == EOF && ferror(file) != 0)
                {
                    snprintf(error_buffer, ERROR_BUFFER_SIZE, "File read error (%s - %d)", filename, ferror(file));
                    result.error = error_buffer;
                    result.status = ASSEMBLER_IO_ERROR;
                }

                fclose(file);
            }
            else
            {
                snprintf(error_buffer, ERROR_BUFFER_SIZE, "Failed to open file (%s)", filename);
                result.error = error_buffer;
                result.status = ASSEMBLER_IO_ERROR;
            }
        }
    }

    if (data.data != 0)
    {
        delete [] data.data;
    }

    if (data.instruction != 0)
    {
        delete [] data.instruction;
    }

    if (data.symbol_table != 0)
    {
        dispose_symbol_table(data.symbol_table);
    }

    return result;
}