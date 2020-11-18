#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <stdint.h>
#include <stdio.h>
#include "symbols.h"
#include "opcodes.h"

#include <errno.h>
#include <iostream>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ASSEMBLER_SUCCESS,
    ASSEMBLER_NOOUTPUT,
    ASSEMBLER_IO_ERROR,
    ASSEMBLER_INTERNAL_ERROR,
    ASSEMBLER_ALLOC_FAILED,
    ASSEMBLER_INPUT_ERROR,
    ASSEMBLER_SYMBOL_ERROR,
    ASSEMBLER_VALUE_OOB,
    ASSEMBLER_INVALID_ARGUMENT,
    ASSEMBLER_SYNTAX_ERROR,
    ASSEMBLER_NO_FREE_ADDRESS_RANGE,
    ASSEMBLER_UNINITIALIZED_VALUE,
} assembler_status_t;

typedef struct _assembler_error
{
    assembler_status_t status;
    int error_start;
    int line_number;
} assembler_error_t;

#ifndef ERROR_BUFFER_SIZE
#define ERROR_BUFFER_SIZE 1024
#endif

typedef struct _assembler_data assembler_data_t;
typedef struct _register_index
{
    register_argument_t *index_register;
    uint8_t is_pre_increment;
    int8_t increment_amount;
} register_index_t;

extern assembler_data_t *assembler_data;
extern char temp_buffer[ERROR_BUFFER_SIZE + 1];
extern const char *current_filename;
extern int *lineNumber;
extern char *error_buffer;

void assemble(const char *filename, const char **search_paths, const char *output_file, assembler_data_t **assembled_data);
assembler_status_t get_starting_executable_address(assembler_data_t *data, uint16_t *address);
assembler_status_t apply_assembled_data_to_buffer(assembler_data_t *data, uint8_t *buffer);
int get_error_buffer_size(assembler_data_t *data);
const char *get_error_buffer(assembler_data_t *data);

// The buffer provided to prepare_executable_file must be at least big enough
// to hold the number of bytes returned by executable_file_size
uint16_t executable_file_size(assembler_data_t *data);
assembler_status_t prepare_executable_file(assembler_data_t *data, uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif // __ASSEMBLER_H__
