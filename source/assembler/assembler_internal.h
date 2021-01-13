#pragma once

#include "assembler.h"

typedef struct _byte_array
{
    uint8_t *array;
    uint16_t size;
} byte_array_t;

typedef struct _assembled_region
{
    uint16_t start_location;
    uint16_t current_instruction_offset;
    // The logical length of this region, so for a .reserve keyword
    // there might not actually be any data to 
    uint16_t length;
    uint16_t data_length;
    uint8_t *data;
    bool executable;
} assembled_region_t;

void add_file_to_process(assembler_data_t *data, const char *included_file);
void handle_file(assembler_data_t *data, const char *included_file);
void handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type);
void handle_org_directive(assembler_data_t *data, uint16_t address);
void handle_instruction(assembler_data_t *data, const opcode_entry_t *opcode, const char *symbol_arg, int literal_arg);
void handle_indexed_instruction(assembler_data_t *data, const opcode_entry_t *opcode, const register_index_t &index_register);
void parse_line(const char *lineBuffer);
byte_array_t add_to_byte_array(byte_array_t array, uint8_t value);
void add_data(assembler_data_t *data, const char *name, byte_array_t array);
void reserve_data(assembler_data_t* data, const char* name, uint16_t size);
void add_string_to_data(assembler_data_t *data, const char *name, const char *string);
void add_error(assembler_data_t *data, const char *error_string, assembler_status_t status);