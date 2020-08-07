#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <stdint.h>
#include <stdio.h>
#include "symbols.h"

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
} assembler_status_t;

typedef struct _assembler_error
{
    assembler_status_t status;
    int error_start;
    int line_number;
} assembler_error_t;

typedef struct _assembler_data
{
    const char **search_paths;
    uint8_t *data;
    size_t data_size;
    uint8_t *instruction;
    size_t instruction_size;
    symbol_table_t *symbol_table;
    int lineNumber;
    const char *current_filename;
    std::vector<char*> files_to_process;
    std::vector<assembler_error_t> errors;
    char * error_buffer;
    int error_buffer_size;
} assembler_data_t;

void assemble(const char *filename, const char **search_paths, const char *output_file, assembler_data_t **assembled_data);

#ifdef __cplusplus
}
#endif

#endif // __ASSEMBLER_H__
