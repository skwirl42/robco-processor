#pragma once

#include <stdint.h>
#include <stdio.h>
#include "symbols.h"
#include "opcodes.h"

#include <errno.h>
#include <iostream>
#include <vector>

enum class assembler_status
{
    SUCCESS,
    NOOUTPUT,
    IO_ERROR,
    INTERNAL_ERROR,
    ALLOC_FAILED,
    INPUT_ERROR,
    SYMBOL_ERROR,
    VALUE_OOB,
    INVALID_ARGUMENT,
    SYNTAX_ERROR,
    NO_FREE_ADDRESS_RANGE,
    UNINITIALIZED_VALUE,
};

enum class assembler_output_type
{
    error,
    none,
    binary,
    summary,
};

struct assembler_error_t
{
    assembler_status status;
    int error_start;
    int line_number;
};

#ifndef ERROR_BUFFER_SIZE
#define ERROR_BUFFER_SIZE 1024
#endif

struct assembler_data_t;

extern assembler_data_t *assembler_data;
extern char temp_buffer[ERROR_BUFFER_SIZE + 1];
extern int *lineNumber;
extern char *error_buffer;

void assemble(const char *filename, const char **search_paths, const char *output_file, assembler_output_type out_file_type, assembler_data_t **assembled_data);
assembler_status get_starting_executable_address(assembler_data_t *data, uint16_t *address);
assembler_status apply_assembled_data_to_buffer(assembler_data_t *data, uint8_t *buffer);
int get_error_buffer_size(assembler_data_t *data);
const char *get_error_buffer(assembler_data_t *data);
const char *get_output_filename(assembler_data_t *data);

// The buffer provided to prepare_executable_file must be at least big enough
// to hold the number of bytes returned by executable_file_size
uint16_t executable_file_size(assembler_data_t *data);
assembler_status prepare_executable_file(assembler_data_t *data, uint8_t *buffer);
