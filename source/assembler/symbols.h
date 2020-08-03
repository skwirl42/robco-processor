#ifndef __SYMBOLS_H__
#define __SYMBOLS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#define SYMBOL_MAX_LENGTH   30

typedef enum SYMBOL_TYPE
{
    SYMBOL_NO_TYPE,
    SYMBOL_WORD,
    SYMBOL_BYTE,
    SYMBOL_ADDRESS_INST,
    SYMBOL_ADDRESS_DATA,
} symbol_type_t;

typedef enum SYMBOL_RESOLUTION
{
    SYMBOL_ASSIGNED,
    SYMBOL_UNASSIGNED,
} symbol_resolution_t;

typedef enum SIGNEDNESS
{
    SIGNEDNESS_ANY,
    SIGNEDNESS_UNSIGNED,
    SIGNEDNESS_SIGNED,
} symbol_signedness_t;

typedef enum SYMBOL_REFERENCE_STATUS
{
    SYMBOL_REFERENCE_SUCCESS,
    SYMBOL_REFERENCE_RESOLVABLE,
    SYMBOL_REFERENCE_WRONG_TYPE,
    SYMBOL_REFERENCE_WRONG_SIGNEDNESS,
    SYMBOL_REFERENCE_UNRESOLVED,
    SYMBOL_REFERENCE_ALLOC_FAILED,
    SYMBOL_REFERENCE_NAMES_EXCEEDED,
} symbol_ref_status_t;

typedef enum SYMBOL_ERROR
{
    SYMBOL_ERROR_NOERROR,
    SYMBOL_ERROR_EXISTS,
    SYMBOL_ERROR_ALLOC_FAILED,
    SYMBOL_ERROR_INTERNAL,
} symbol_error_t;

typedef enum SYMBOL_TABLE_ERROR
{
    SYMBOL_TABLE_NOERROR,
    SYMBOL_TABLE_ALLOC_FAILED,
} symbol_table_error_t;

typedef struct _symbol_table symbol_table_t;

symbol_table_error_t create_symbol_table(symbol_table_t **symbol_table);
symbol_table_error_t dispose_symbol_table(symbol_table_t *symbol_table);
symbol_error_t add_symbol(symbol_table_t *symbol_table, const char *name, symbol_type_t type, symbol_signedness_t signedness, uint16_t word_value, uint8_t byte_value);
symbol_ref_status_t add_symbol_reference(symbol_table_t *symbol_table, const char *name, uint8_t *reference_address, uint16_t ref_location, symbol_signedness_t expected_signedness, symbol_type_t expected_type);
symbol_resolution_t resolve_symbol(symbol_table_t *symbol_table, const char *name, symbol_type_t *symbol_type, symbol_signedness_t *signedness, uint16_t *word_value, uint8_t *byte_value);
void output_symbols(FILE *output_file, symbol_table_t *symbol_table);

// on enter
// count = number of strings of at least SYMBOL_MAX_LENGTH passed into 'symbols'
// symbols = 'count' empty strings of at least SYMBOL_MAX_LENGTH
//
// on exit
// count = number of actual strings filled
// symbols = 'count' strings will be filled with symbol names
symbol_ref_status_t check_all_symbols_resolved(symbol_table_t *symbol_table, int *unresolved_name_count, char **unresolved_symbol_names);

#ifdef __cplusplus
}
#endif

#endif // __SYMBOLS_H__