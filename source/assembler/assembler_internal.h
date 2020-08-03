#pragma once

#include "assembler.h"
#include "opcodes.h"

typedef struct _byte_array
{
    uint8_t *array;
    size_t size;
} byte_array_t;

extern assembler_data_t *assembler_data;

void add_file_to_process(assembler_data_t *data, const char *included_file);
assembler_result_t handle_file(assembler_data_t *data, const char *included_file);
assembler_result_t handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type);
assembler_result_t handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg);
void parse_line(const char *lineBuffer);
byte_array_t add_to_byte_array(byte_array_t array, uint8_t value);
assembler_result_t add_data(assembler_data_t *data, const char *name, byte_array_t array);
