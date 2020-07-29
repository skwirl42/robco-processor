#pragma once

#include "assembler.h"

#include <stdint.h>
#include <stdio.h>
#include "symbols.h"

#include <errno.h>
#include <regex>
#include <iostream>

#define ERROR_BUFFER_SIZE   1024
#define LINE_BUFFER_SIZE    1024

typedef struct opcode_entry
{
    const char *name;
    uint8_t opcode;
    int arg_byte_count;
    symbol_type_t argument_type;
    bool use_specified_operand;
    uint8_t operands;
} opcode_entry_t;

typedef struct _assembler_data
{
    const char **search_paths;
    uint8_t *data;
    size_t data_size;
    uint8_t *instruction;
    size_t instruction_size;
    symbol_table_t *symbol_table;
    int lineNumber;
} assembler_data_t;

extern assembler_data_t *assembler_data;

assembler_result_t handle_include(assembler_data_t *data, const char *included_file);
assembler_result_t handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type);
assembler_result_t handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg);
opcode_entry_t *get_opcode_entry(const char *opcode_name);
void parse_line(const char *lineBuffer);
