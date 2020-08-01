#pragma once

#include "assembler.h"

#define ERROR_BUFFER_SIZE   1024
#define LINE_BUFFER_SIZE    1024

typedef struct opcode_entry
{
    const char *name;
    uint8_t opcode;
    int arg_byte_count;
    symbol_type_t argument_type;
    symbol_signedness_t argument_signedness;
    bool use_specified_operand;
    uint8_t operands;
} opcode_entry_t;

typedef union
{
    uint16_t uword;
    int16_t sword;
    uint8_t bytes[2];
} assembler_word_t;

extern assembler_data_t *assembler_data;

void add_file_to_process(assembler_data_t *data, const char *included_file);
assembler_result_t handle_file(assembler_data_t *data, const char *included_file);
assembler_result_t handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type);
assembler_result_t handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg);
opcode_entry_t *get_opcode_entry(const char *opcode_name);
void parse_line(const char *lineBuffer);
