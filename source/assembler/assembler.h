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
    ASSEMBLER_WARNINGS,
    ASSEMBLER_ERRORS,
    ASSEMBLER_ALLOC_FAILED,
    ASSEMBLER_INPUT_ERROR,
    ASSEMBLER_SYMBOL_ERROR,
    ASSEMBLER_VALUE_OOB,
} assembler_status_t;

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
} assembler_data_t;

typedef struct _assembler_result
{
    assembler_status_t status;
    int warning_count;
    int error_count;
    const char *error;
} assembler_result_t;

assembler_result_t assemble(const char *filename, const char **search_paths, assembler_data_t **assembled_data);

#ifdef __cplusplus
}
#endif

#endif // __ASSEMBLER_H__
