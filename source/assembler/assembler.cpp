#include "assembler.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <regex>
#include <iostream>

#include "opcodes.h"
#include "symbols.h"
#include "memory.h"

typedef struct opcode_entry
{
    const char *name;
    uint8_t opcode;
    int arg_byte_count;
    bool use_specified_operand;
    uint8_t operands;
} opcode_entry_t;

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
    { "pushi", OPCODE_PUSHI, 1 },
    { "pushiw", OPCODE_PUSHI + OPCODE_SIZE_BIT, 2 },
    { "pop", OPCODE_POP, 1 },
    { "popw", OPCODE_POP + OPCODE_SIZE_BIT, 1 },
    { "dup", OPCODE_DUP, 1 },
    { "dupw", OPCODE_DUP + OPCODE_SIZE_BIT, 1 },
    { "swap", OPCODE_SWAP, 1 },
    { "swapw", OPCODE_SWAP + OPCODE_SIZE_BIT, 1 },
    { "roll", OPCODE_ROLL, 1 },
    { "rollw", OPCODE_ROLL + OPCODE_SIZE_BIT, 1 },
    { "pushsi", OPCODE_PUSH, 1, true, (STACK_INDEX << 6) },
    { "pushdp", OPCODE_PUSH, 1, true, (DIRECT_PAGE << 6) },
    { "pushx", OPCODE_PUSH + OPCODE_SIZE_BIT, 1, true, (X_REGISTER << 6) },
    { "pullsi", OPCODE_PULL, 1, true, STACK_INDEX },
    { "pulldp", OPCODE_PULL, 1, true, DIRECT_PAGE },
    { "pullx", OPCODE_PULL + OPCODE_SIZE_BIT, 1, true, X_REGISTER },

    // Flow instructions
    { "b", OPCODE_B, 2 },
    { "be", OPCODE_BE, 2 },
    { "bn", OPCODE_BN, 2 },
    { "bc", OPCODE_BC, 2 },
    { "bo", OPCODE_BO, 2 },
    { "jmp", OPCODE_JMP, 2 },
    { "jsr", OPCODE_JSR, 2 },
    { "rts", OPCODE_RTS, 1 },
    { "syscall", OPCODE_SYSCALL, 2 },

    { 0, 0, 0 }
};

const char *include_pattern = "^\\.include\\s+\"(.+)\"";
const char *defword_pattern = "^\\.defword\\s+(\\S+)\\s+(\\S+)";
const char *defbyte_pattern = "^\\.defbyte\\s+(\\S+)\\s+(\\S+)";
const char *label_pattern = "^(\\w+):";
const char *instruction_pattern = "^\\s+(\\w+)\\s+(\\w+)?";
const char *data_statement_pattern = "^\\.data\\s+((\\w+)\\s+)+";

#define ERROR_BUFFER_SIZE   1024
#define LINE_BUFFER_SIZE    1024

static char error_buffer[ERROR_BUFFER_SIZE + 1];

typedef struct _assembler_data
{
    const char **search_paths;
    uint8_t *data;
    size_t data_size;
    uint8_t *instruction;
    size_t instruction_size;
    symbol_table_t *symbol_table;
} assembler_data_t;

assembler_result_t handle_include(assembler_data_t *data, const char *included_file)
{
    // These are the only statements that can be present in an include
    std::regex defword_regex(defword_pattern, std::regex_constants::icase);
    std::regex defbyte_regex(defbyte_pattern, std::regex_constants::icase);

    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    return result;
}

void output_results(std::cmatch &results, bool do_output = false)
{
    if (do_output)
    {
        for (auto it = results.begin(); it != results.end(); it++)
        {
            std::cout << "---- " << it->str() << std::endl;
        }
    }
}

assembler_result_t handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    result.error = error_buffer;

    fprintf(stdout, "-> %s = %d\n", name, value);
    symbol_error_t sym_err;
    switch (type)
    {
    case SYMBOL_WORD:
        sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, value, 0);
        break;

    case SYMBOL_BYTE:
        sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, 0, value);
        break;

    case SYMBOL_ADDRESS:
        sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, value, 0);
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

    std::regex include_regex(include_pattern, std::regex_constants::icase);
    std::regex defword_regex(defword_pattern, std::regex_constants::icase);
    std::regex defbyte_regex(defbyte_pattern, std::regex_constants::icase);
    std::regex label_regex(label_pattern, std::regex_constants::icase);
    std::regex instruction_regex(instruction_pattern, std::regex_constants::icase);
    std::regex data_regex(data_statement_pattern, std::regex_constants::icase);

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
                int lineNumber = 0;
                char lineBuffer[LINE_BUFFER_SIZE + 1];
                int charIndex = 0;
                int currentChar;
                while ((currentChar = fgetc(file)) != EOF)
                {
                    if (currentChar == '\r' || currentChar == '\n' || currentChar == EOF)
                    {
                        lineBuffer[charIndex] = 0;
                        // Process the line
                        if (charIndex > 0)
                        {
                            // fprintf(stdout, "Working on line %d\n", lineNumber);
                            std::cmatch results;
                            // TODO: Test regexes on the line
                            if (std::regex_search(lineBuffer, results, include_regex))
                            {
                                auto include_result = handle_include(&data, results[1].str().c_str());
                                output_results(results);
                            }
                            else if (std::regex_search(lineBuffer, results, defword_regex))
                            {
                                int num_value;
                                sscanf(results[2].str().c_str(), "0x%x", &num_value);
                                auto def_result = handle_symbol_def(&data, results[1].str().c_str(), num_value, SYMBOL_WORD);
                                output_results(results);
                            }
                            else if (std::regex_search(lineBuffer, results, defbyte_regex))
                            {
                                int num_value;
                                sscanf(results[2].str().c_str(), "0x%x", &num_value);
                                auto def_result = handle_symbol_def(&data, results[1].str().c_str(), num_value, SYMBOL_BYTE);
                                output_results(results);
                            }
                            else if (std::regex_search(lineBuffer, results, label_regex))
                            {
                                auto label_result = handle_symbol_def(&data, results[1].str().c_str(), data.instruction_size, SYMBOL_ADDRESS);
                                output_results(results);
                            }
                            else if (std::regex_search(lineBuffer, results, data_regex))
                            {
                                output_results(results);
                            }
                            else if (std::regex_search(lineBuffer, results, instruction_regex))
                            {
                                const char *instruction = results[1].str().c_str();
                                auto opcode_entry = get_opcode_entry(instruction);

                                if (opcode_entry == 0)
                                {
                                    snprintf(error_buffer, ERROR_BUFFER_SIZE, "Unknown instruction %s", instruction);
                                    result.status = ASSEMBLER_ERRORS;
                                }
                                else
                                {
                                    const char *operand = 0;
                                    if (results[2].matched)
                                    {
                                        operand = results[2].str().c_str();
                                    }

                                    if (operand != 0)
                                    {
                                        symbol_type_t type;
                                        symbol_signedness_t signedness;
                                        uint16_t word_value;
                                        uint8_t byte_value;
                                        symbol_resolution_t symbol_res = resolve_symbol(data.symbol_table, operand, &type, &signedness, &word_value, &byte_value);
                                        if (symbol_res == SYMBOL_ASSIGNED)
                                        {
                                            switch (type)
                                            {
                                            case SYMBOL_ADDRESS:
                                                fprintf(stdout, "Got address %s with value 0x%04x\n", operand, word_value);
                                                break;

                                            case SYMBOL_BYTE:
                                                fprintf(stdout, "Got byte %s with value 0x%02x\n", operand, byte_value);
                                                break;

                                            case SYMBOL_WORD:
                                                fprintf(stdout, "Got word %s with value 0x%04x\n", operand, word_value);
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            // Could be an immediate value, but only with pushi
                                            fprintf(stderr, "Failed to find symbol %s\n", operand);
                                        }
                                    }
                                }
                                
                                output_results(results);
                            }
                            else
                            {
                                snprintf(error_buffer, ERROR_BUFFER_SIZE, "Unexpected line at %d", lineNumber);
                                result.error = error_buffer;
                                result.status = ASSEMBLER_INPUT_ERROR;
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