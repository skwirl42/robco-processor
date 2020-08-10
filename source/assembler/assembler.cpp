#include "assembler_internal.h"

#include <errno.h>
#include "opcodes.h"
#include "memory.h"

static char error_buffer[ERROR_BUFFER_SIZE + 1];
char temp_buffer[ERROR_BUFFER_SIZE + 1];
assembler_data_t *assembler_data;

byte_array_t add_to_byte_array(byte_array_t array, uint8_t value)
{
    auto old_array = array.array;
    auto old_size = array.size;
    array.array = new uint8_t[array.size + 1];
    memcpy(array.array, old_array, array.size);
    array.array[array.size++] = value;
    if (old_size > 0 && old_array != nullptr)
    {
        delete [] old_array;
    }
    return array;
}

void add_file_to_process(assembler_data_t *data, const char *filename)
{
    char *new_file = new char[LINE_BUFFER_SIZE + 1];
    strncpy(new_file, filename, LINE_BUFFER_SIZE);
    data->files_to_process.push_back(new_file);
}

void add_data(assembler_data_t *data, const char *name, byte_array_t array)
{
    if (name != nullptr)
    {
        handle_symbol_def(data, name, 0, SYMBOL_ADDRESS_DATA);
    }

    memcpy(&data->data[data->data_size], array.array, array.size);
    data->data_size += array.size;

    array.size = 0;
    delete [] array.array;
    array.array = nullptr;
}

void add_string_to_data(assembler_data_t *data, const char *name, const char *string)
{
    if (name != nullptr)
    {
        handle_symbol_def(data, name, 0, SYMBOL_ADDRESS_DATA);
    }

    memcpy(&data->data[data->data_size], string, strlen(string) + 1);
    data->data[data->data_size + strlen(string)] = 0;
    data->data_size += strlen(string) + 1;
}

void handle_file(assembler_data_t *data, const char *filename)
{
    auto old_filename = data->current_filename;
    auto old_line_number = data->lineNumber;
    data->current_filename = filename;

    FILE *file = fopen(filename, "r");

    if (file == 0 && data->search_paths != 0)
    {
        char * path_buffer = new char[ERROR_BUFFER_SIZE + 1];
        const char * current_search_path = 0;
        int i = 0;
        while ((current_search_path = data->search_paths[i++]) != 0)
        {
            strncpy(path_buffer, current_search_path, ERROR_BUFFER_SIZE);
            strncat(path_buffer, filename, ERROR_BUFFER_SIZE);
            file = fopen(path_buffer, "r");
            if (file != 0)
            {
                break;
            }
        }
        delete [] path_buffer;
    }

    if (file != 0)
    {
        // printf("Processing %s\n", filename);
        int lineNumber = 1;
        char lineBuffer[LINE_BUFFER_SIZE + 1];
        int charIndex = 0;
        int currentChar;
        bool had_eof = false;
        while (!had_eof && (currentChar = fgetc(file)))
        {
            if (currentChar == EOF)
            {
                had_eof = true;
            }

            if (currentChar == '\r' || currentChar == '\n' || currentChar == EOF)
            {
                data->lineNumber = lineNumber;
                lineBuffer[charIndex] = 0;
                // Process the line
                if (charIndex > 0)
                {
                    parse_line(lineBuffer);
                    // fprintf(stdout, "Got line %d: %s\n", lineNumber, lineBuffer);
                }

                if (data->files_to_process.size() > 0)
                {
                    auto it = data->files_to_process.begin();
                    while (it != data->files_to_process.end())
                    {
                        auto file = *it;
                        data->files_to_process.erase(data->files_to_process.begin());
                        it = data->files_to_process.begin();
                        handle_file(data, file);
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
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Line %d is too long", lineNumber);
                add_error(data, temp_buffer, ASSEMBLER_INPUT_ERROR);
            }
            else
            {
                lineBuffer[charIndex++] = currentChar;
            }
        }

        if (currentChar == EOF && ferror(file) != 0)
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "File read error (%s - %d)", filename, ferror(file));
            add_error(data, temp_buffer, ASSEMBLER_IO_ERROR);
        }

        fclose(file);
    }
    else
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Failed to open file (%s)", filename);
        fprintf(stderr, "%s\n", temp_buffer);
        add_error(data, temp_buffer, ASSEMBLER_IO_ERROR);
    }

    data->lineNumber = old_line_number;
    data->current_filename = old_filename;
}

void handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type)
{
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

    if (sym_err == SYMBOL_ERROR_ALLOC_FAILED)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Couldn't allocate space for a symbol named %s", name);
        add_error(data, temp_buffer, ASSEMBLER_ALLOC_FAILED);
    }
    else if (sym_err == SYMBOL_ERROR_EXISTS)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Tried to redefine a symbol named %s", name);
        add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
    }
    else if (sym_err == SYMBOL_ERROR_INTERNAL)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Internal error trying to define a symbol named %s", name);
        add_error(data, temp_buffer, ASSEMBLER_INTERNAL_ERROR);
    }
}

void handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg)
{
    // printf("Handling instruction %s with arg (%s)\n", opcode->name, symbol_arg ? symbol_arg : "numerical");
    if (opcode->access_mode == STACK_ONLY && (symbol_arg != nullptr || literal_arg != 0))
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Stack-only instruction %s cannot take parameters", opcode->name);
        add_error(data, temp_buffer, ASSEMBLER_INVALID_ARGUMENT);
    }
    else if (opcode->access_mode == REGISTER_INDEXED)
    {
        if (SOURCE_2(literal_arg) == 0)
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Register indexed instruction %s requires a register to be specified", opcode->name);
            add_error(data, temp_buffer, ASSEMBLER_INVALID_ARGUMENT);
        }
        else
        {
            data->instruction[data->instruction_size++] = opcode->opcode;
            data->instruction[data->instruction_size++] = (uint8_t)literal_arg;
        }
    }
    else if (symbol_arg)
    {
        symbol_type_t type;
        symbol_signedness_t signedness = SIGNEDNESS_ANY;
        uint16_t word_value;
        uint8_t byte_value;
        auto resolution = resolve_symbol(data->symbol_table, symbol_arg, &type, &signedness, &word_value, &byte_value);
        if (resolution != SYMBOL_ASSIGNED)
        {
            symbol_ref_status_t add_ref_result;
            if (opcode->argument_type == SYMBOL_ADDRESS_INST)
            {
                if (IS_BRANCH_INST(opcode->opcode))
                {
                    signedness = SIGNEDNESS_SIGNED;
                }
                add_ref_result = add_symbol_reference(data->symbol_table, symbol_arg, 
                                                        &data->instruction[data->instruction_size + 1], data->instruction_size + 1, 
                                                        signedness, SYMBOL_ADDRESS_INST);
            }
            else 
            {
                add_ref_result = add_symbol_reference(data->symbol_table, symbol_arg,
                                                        &data->instruction[data->instruction_size + 1], data->instruction_size + 1, 
                                                        opcode->argument_signedness, opcode->argument_type);
            }

            if (add_ref_result != SYMBOL_REFERENCE_RESOLVABLE && add_ref_result != SYMBOL_REFERENCE_SUCCESS)
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Error trying to add a reference to symbol %s", symbol_arg);
                add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
            }
            data->symbol_references_count++;
        }
        machine_word_t word;
        word.uword = word_value;
        if (resolution == SYMBOL_ASSIGNED)
        {
            switch (type)
            {
            case SYMBOL_WORD:
            case SYMBOL_ADDRESS_DATA:
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
                    word.sword -= (int16_t)data->instruction_size;
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

            case SYMBOL_NO_TYPE:
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Error resolving symbol %s", symbol_arg);
                add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
                break;
            }
        }
        else if (opcode->argument_type == SYMBOL_ADDRESS_INST)
        {
            data->instruction[data->instruction_size++] = opcode->opcode;
            data->instruction[data->instruction_size++] = 0;
            data->instruction[data->instruction_size++] = 0;
        }
    }
    else if (opcode->use_specified_operand)
    {
        data->instruction[data->instruction_size++] = opcode->opcode;
        data->instruction[data->instruction_size++] = opcode->operands;
    }
    else if (opcode->argument_type == SYMBOL_NO_TYPE)
    {
        // There may be register specifications in the literal arg
        data->instruction[data->instruction_size++] = opcode->opcode;
        data->instruction[data->instruction_size++] = (uint8_t)literal_arg;

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
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Opcode %s cannot accommodate a value of %d", opcode->name, literal_arg);
            add_error(data, temp_buffer, ASSEMBLER_VALUE_OOB);
        }
        else if (opcode->argument_type == SYMBOL_WORD)
        {
            if (literal_arg < 0)
            {
                literal_arg += 65536;
            }

            uint16_t literal = literal_arg;
            machine_word_t word;
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
}

void add_error(assembler_data_t *data, const char *error_string, assembler_status_t status)
{
    assembler_error_t error;
    error.status = status;
    error.line_number = data->lineNumber;
    error.error_start = data->error_buffer_size;
    int length = snprintf(&data->error_buffer[error.error_start], ERROR_BUFFER_SIZE - data->error_buffer_size, "%s:%d - %s\n", data->current_filename, data->lineNumber, error_string);
    data->errors.push_back(error);
    data->error_buffer_size += length;
}

assembler_data_t data;
void assemble(const char *filename, const char **search_paths, const char *output_file, assembler_data_t **assembled_data)
{
    data.search_paths = search_paths;
    data.data = new uint8_t[DATA_SIZE];
    data.data_size = 0;
    data.instruction = new uint8_t[INST_SIZE];
    data.instruction_size = 0;
    data.lineNumber = 0;
    data.symbol_references_count = 0;
    assembler_data = &data;

    error_buffer[0] = 0;
    data.error_buffer = error_buffer;
    data.error_buffer_size = 0;
    *assembled_data = assembler_data;

    if (create_symbol_table(&data.symbol_table) != SYMBOL_TABLE_NOERROR)
    {
        add_error(&data, "Failed to create the symbol table", ASSEMBLER_ALLOC_FAILED);
        return;
    }

    handle_file(&data, filename);

    if (data.symbol_references_count > 0)
    {
        data.current_filename = filename;
        char **symbol_buffers = new char*[data.symbol_references_count];

        for (int i = 0; i < data.symbol_references_count; i++)
        {
            symbol_buffers[i] = new char[SYMBOL_MAX_LENGTH + 1];
        }

        int symbol_count = data.symbol_references_count;
        auto result = check_all_symbols_resolved(data.symbol_table, &symbol_count, symbol_buffers);

        if (result != SYMBOL_REFERENCE_RESOLVABLE)
        {
            for (int i = 0; i < symbol_count; i++)
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Unresolved symbol %s", symbol_buffers[i]);
                add_error(&data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
            }
        }

        for (int i = 0; i < data.symbol_references_count; i++)
        {
            delete [] symbol_buffers[i];
        }

        delete [] symbol_buffers;
    }

    if (data.errors.size() > 0)
    {
        return;
    }

    if (output_file == nullptr)
    {
        output_file = "assembled.txt";
    }
    
    FILE *assembled_output = fopen(output_file, "w+");
    if (assembled_output != 0)
    {
        fprintf(assembled_output, "Code:\n");
        for (int i = 0; i < data.instruction_size;)
        {
            fprintf(assembled_output, "0x%04x: ", i);
            for (int j = 0; j < 4 && i < data.instruction_size; j++)
            {
                fprintf(assembled_output, "0x%02x ", data.instruction[i++]);
            }
            fprintf(assembled_output, "\n");
        }

        fprintf(assembled_output, "\nData:\n");
        for (int i = 0; i < data.data_size;)
        {
            fprintf(assembled_output, "0x%04x: ", i);
            for (int j = 0; j < 4 && i < data.data_size; j++)
            {
                fprintf(assembled_output, "0x%02x ", data.data[i++]);
            }
            fprintf(assembled_output, "\n");
        }

        fprintf(assembled_output, "\nSymbols:\n");
        output_symbols(assembled_output, data.symbol_table);
    }
    else
    {
        fprintf(stderr, "Couldn't open assembler output file %s (errno: %d)\n", output_file, errno);
    }

    if (data.symbol_table != 0)
    {
        dispose_symbol_table(data.symbol_table);
    }
}