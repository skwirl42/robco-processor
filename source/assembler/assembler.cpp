#include "assembler.h"

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <regex>

#include "opcodes.h"
#include "symbols.h"

struct opcode_entry
{
    const char *name;
    uint8_t opcode;
    int arg_bytes;
};

struct opcode_entry opcode_entries[] =
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
const char *instruction_pattern = "^\\s+(\\w+)\\s+(\\w+)";

#define ERROR_BUFFER_SIZE   1024
#define LINE_BUFFER_SIZE    1024
#define OVEC_SIZE           30

static char error_buffer[ERROR_BUFFER_SIZE + 1];

typedef struct _assembler_data
{
    symbol_table_t *symbol_table;
} assembler_data_t;

assembler_result_t handle_include(assembler_data_t *data)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    return result;
}

assembler_result_t assemble(const char *filename, const char **search_paths)
{
    assembler_result_t result;
    result.status = ASSEMBLER_NOOUTPUT;
    assembler_data_t data;

    // PCRE initialization
    const char *error;
    int error_offset;
    
    std::regex include_regex(include_pattern);

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
                            // TODO: Execute regex on line
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

    if (data.symbol_table != 0)
    {
        dispose_symbol_table(data.symbol_table);
    }

    return result;
}