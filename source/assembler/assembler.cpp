#include "assembler_internal.h"

#include "opcodes.h"
#include "memory.h"
#include "emulator.h"

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
    { "pushsi", OPCODE_PUSH,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         true, (STACK_INDEX << 6) },
    { "pushdp", OPCODE_PUSH,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         true, (DIRECT_PAGE << 6) },
    { "pushx",  OPCODE_PUSH + OPCODE_SIZE_BIT,  1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         true, (X_REGISTER << 6) },
    { "pullsi", OPCODE_PULL,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         true, STACK_INDEX },
    { "pulldp", OPCODE_PULL,                    1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         true, DIRECT_PAGE },
    { "pullx",  OPCODE_PULL + OPCODE_SIZE_BIT,  1, SYMBOL_NO_TYPE,      SIGNEDNESS_ANY,         true, X_REGISTER },

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

static char error_buffer[ERROR_BUFFER_SIZE + 1];
assembler_data_t *assembler_data;

void add_file_to_process(assembler_data_t *data, const char *filename)
{
    char *new_file = new char[LINE_BUFFER_SIZE + 1];
    strncpy(new_file, filename, LINE_BUFFER_SIZE);
    data->files_to_process.push_back(new_file);
}

assembler_result_t handle_file(assembler_data_t *data, const char *filename)
{
    auto old_filename = data->current_filename;
    auto old_line_number = data->lineNumber;
    data->current_filename = filename;

    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;

    FILE *file = fopen(filename, "r");

    if (file == 0 && data->search_paths != 0)
    {
        char * path_buffer = new char[ERROR_BUFFER_SIZE + 1];
        const char * current_search_path = 0;
        int i = 0;
        while ((current_search_path = data->search_paths[i]) != 0)
        {
            strncpy(path_buffer, current_search_path, ERROR_BUFFER_SIZE);
            strncat(path_buffer, filename, ERROR_BUFFER_SIZE);
            file = fopen(path_buffer, "r");
            if (file != 0)
            {
                break;
            }
        }
    }

    if (file != 0)
    {
        // printf("Processing %s\n", filename);
        int lineNumber = 1;
        char lineBuffer[LINE_BUFFER_SIZE + 1];
        int charIndex = 0;
        int currentChar;
        while ((currentChar = fgetc(file)) != EOF)
        {
            if (currentChar == '\r' || currentChar == '\n' || currentChar == EOF)
            {
                data->lineNumber = lineNumber;
                lineBuffer[charIndex] = 0;
                // Process the line
                if (charIndex > 0)
                {
                    parse_line(lineBuffer);
                }

                if (data->files_to_process.size() > 0)
                {
                    auto it = data->files_to_process.begin();
                    while (it != data->files_to_process.end())
                    {
                        auto file = *it;
                        data->files_to_process.erase(data->files_to_process.begin());
                        it = data->files_to_process.begin();
                        auto include_result = handle_file(data, file);
                        delete [] file;
                    }
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
        fprintf(stderr, "%s\n", error_buffer);
        result.error = error_buffer;
        result.status = ASSEMBLER_IO_ERROR;
    }

    data->lineNumber = old_line_number;
    data->current_filename = old_filename;
    return result;
}

assembler_result_t handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;

    if (type == SYMBOL_ADDRESS_INST)
    {
        value = data->instruction_size;
    }

    if (type == SYMBOL_ADDRESS_DATA)
    {
        // the data segment will get written to after the direct page area
        value = data->data_size + 0x100;
    }

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

    case SYMBOL_NO_TYPE:
        sym_err = SYMBOL_ERROR_INTERNAL;
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
    else if (sym_err == SYMBOL_ERROR_INTERNAL)
    {
        snprintf(error_buffer, ERROR_BUFFER_SIZE, "Internal error trying to define a symbol named %s", name);
        result.status = ASSEMBLER_INTERNAL_ERROR;
    }

    return result;
}

opcode_entry_t *get_opcode_entry(const char *opcode_name)
{
    int index = 0;
    opcode_entry_t *result;
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

assembler_result_t handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;

    // printf("Handling instruction %s with arg (%s)\n", opcode->name, symbol_arg ? symbol_arg : "numerical");
    if (symbol_arg)
    {
        symbol_type_t type;
        symbol_signedness_t signedness;
        uint16_t word_value;
        uint8_t byte_value;
        auto resolution = resolve_symbol(data->symbol_table, symbol_arg, &type, &signedness, &word_value, &byte_value);
        assembler_word_t word;
        word.uword = word_value;
        if (resolution == SYMBOL_ASSIGNED)
        {
            switch (type)
            {
            case SYMBOL_WORD:
                data->instruction[data->instruction_size++] = opcode->opcode;
                data->instruction[data->instruction_size++] = word.bytes[1];
                data->instruction[data->instruction_size++] = word.bytes[0];
                break;

            case SYMBOL_BYTE:
                data->instruction[data->instruction_size++] = opcode->opcode;
                data->instruction[data->instruction_size++] = byte_value;
                break;

            case SYMBOL_ADDRESS_INST:
                if (opcode->argument_signedness == SIGNEDNESS_SIGNED)
                {
                    auto target = (int16_t)word_value - (int16_t)data->instruction_size;
                    word.sword = target;
                    // fprintf(stdout, "Branching by %d, from %zu to %u\n", target, data->instruction_size + 3, word_value);
                    data->instruction[data->instruction_size++] = opcode->opcode;
                    data->instruction[data->instruction_size++] = word.bytes[1];
                    data->instruction[data->instruction_size++] = word.bytes[0];
                }
                else
                {
                    data->instruction[data->instruction_size++] = opcode->opcode;
                    data->instruction[data->instruction_size++] = word.bytes[1];
                    data->instruction[data->instruction_size++] = word.bytes[0];
                }
                break;

            case SYMBOL_ADDRESS_DATA:
                printf("Got value %04x for data address symbol %s\n", word_value, symbol_arg);
                break;

            case SYMBOL_NO_TYPE:
                snprintf(error_buffer, ERROR_BUFFER_SIZE, "Error resolving symbol %s", symbol_arg);
                result.error = error_buffer;
                result.status = ASSEMBLER_SYMBOL_ERROR;
                break;
            }
        }
        else
        {
            snprintf(error_buffer, ERROR_BUFFER_SIZE, "Could not resolve symbol %s", symbol_arg);
            result.error = error_buffer;
            result.status = ASSEMBLER_SYMBOL_ERROR;
        }
    }
    else if (opcode->use_specified_operand)
    {
        data->instruction[data->instruction_size++] = opcode->opcode;
        data->instruction[data->instruction_size++] = opcode->operands;
    }
    else if (opcode->argument_type == SYMBOL_NO_TYPE)
    {
        data->instruction[data->instruction_size++] = opcode->opcode;
        data->instruction[data->instruction_size++] = 0;

        if (opcode->arg_byte_count > 1)
        {
            data->instruction[data->instruction_size++] = 0;
        }
    }
    else
    {
        // handle literal
        // Bounds check
        if ((opcode->argument_type == SYMBOL_WORD && (literal_arg > 65535 || literal_arg < -32768))
            || (opcode->argument_type == SYMBOL_BYTE && (literal_arg > 255 || literal_arg < -128)))
        {
            snprintf(error_buffer, ERROR_BUFFER_SIZE, "Opcode %s cannot accommodate a value of %d", opcode->name, literal_arg);
            result.error = error_buffer;
            result.status = ASSEMBLER_VALUE_OOB;
        }
        else if (opcode->argument_type == SYMBOL_WORD)
        {
            if (literal_arg < 0)
            {
                literal_arg += 65536;
            }

            uint16_t literal = literal_arg;
            assembler_word_t word;
            word.uword = literal;

            data->instruction[data->instruction_size++] = opcode->opcode;
            data->instruction[data->instruction_size++] = word.bytes[1];
            data->instruction[data->instruction_size++] = word.bytes[0];
        }
        else
        {
            if (literal_arg < 0)
            {
                literal_arg += 256;
            }

            data->instruction[data->instruction_size++] = opcode->opcode;
            data->instruction[data->instruction_size++] = literal_arg;
        }
        
    }

    return result;
}

assembler_data_t data;
assembler_result_t assemble(const char *filename, const char **search_paths, assembler_data_t **assembled_data)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;
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
            auto file_result = handle_file(&data, filename);
            result.error = file_result.error;
            result.warning_count = file_result.warning_count;
            result.error_count = file_result.error_count;
            result.status = file_result.status;

            for (int i = 0; i < data.instruction_size; i++)
            {
                fprintf(stdout, "0x%02x\n", data.instruction[i]);
            }
        }
    }

    *assembled_data = &data;

    if (data.symbol_table != 0)
    {
        dispose_symbol_table(data.symbol_table);
    }

    return result;
}