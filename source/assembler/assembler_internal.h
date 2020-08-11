#pragma once

#include "assembler.h"
#include "opcodes.h"

typedef struct _byte_array
{
    uint8_t *array;
    size_t size;
} byte_array_t;

extern assembler_data_t *assembler_data;
extern char temp_buffer[ERROR_BUFFER_SIZE + 1];

void add_file_to_process(assembler_data_t *data, const char *included_file);
void handle_file(assembler_data_t *data, const char *included_file);
void handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type);
void handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg);
void parse_line(const char *lineBuffer);
byte_array_t add_to_byte_array(byte_array_t array, uint8_t value);
void add_data(assembler_data_t *data, const char *name, byte_array_t array);
void reserve_data(assembler_data_t* data, const char* name, uint16_t size);
void add_string_to_data(assembler_data_t *data, const char *name, const char *string);
void add_error(assembler_data_t *data, const char *error_string, assembler_status_t status);