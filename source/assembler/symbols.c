#include "symbols.h"

#include <string.h>
#include <stdlib.h>

typedef struct _symbol_reference symbol_reference_t;
typedef struct _symbol_table_entry symbol_table_entry_t;

struct _symbol_reference
{
    char symbol[SYMBOL_MAX_LENGTH + 1];
    symbol_reference_t *next_reference;
    uint16_t location;
    symbol_resolution_t resolution;
    symbol_signedness_t expected_signedness;
    symbol_type_t expected_type;
};

struct _symbol_table_entry
{
    char symbol[SYMBOL_MAX_LENGTH + 1];
    symbol_table_entry_t *next_entry;
    symbol_type_t type;
    symbol_signedness_t signedness;
    uint16_t word_value;
    uint8_t word_value_valid;
    uint8_t byte_value;
    uint8_t byte_value_valid;
};

struct _symbol_table
{
    symbol_table_entry_t *first_entry;
    symbol_reference_t *first_reference;
};

symbol_table_error_t create_symbol_table(symbol_table_t **symbol_table)
{
    *symbol_table = malloc(sizeof(struct _symbol_table));
    if (symbol_table == USER_ADDR_NULL)
    {
        return SYMBOL_TABLE_ALLOC_FAILED;
    }

    (*symbol_table)->first_entry = 0;
    (*symbol_table)->first_reference = 0;

    return SYMBOL_TABLE_NOERROR;
}

symbol_table_error_t dispose_symbol_table(symbol_table_t *symbol_table)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry;
    while (current_entry != 0)
    {
        next_entry = current_entry->next_entry;
        free(current_entry);
        current_entry = next_entry;
    }

    symbol_reference_t *current_ref = symbol_table->first_reference;
    symbol_reference_t *next_ref;
    while (current_ref != USER_ADDR_NULL)
    {
        next_ref = current_ref->next_reference;
        free(current_ref);
        current_ref = next_ref;
    }

    free(symbol_table);

    return SYMBOL_TABLE_NOERROR;
}

symbol_error_t add_symbol(symbol_table_t *symbol_table, const char *name, symbol_type_t type, symbol_signedness_t signedness, uint16_t word_value, uint8_t byte_value)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry = 0;
    symbol_table_entry_t *last_entry = 0;
    while (current_entry != USER_ADDR_NULL)
    {
        next_entry = current_entry->next_entry;
        if (strncasecmp(name, current_entry->symbol, SYMBOL_MAX_LENGTH) == 0)
        {
            return SYMBOL_ERROR_EXISTS;
        }
        last_entry = current_entry;
        current_entry = next_entry;
    }

    symbol_table_entry_t *new_entry = malloc(sizeof(symbol_table_entry_t));
    if (new_entry != USER_ADDR_NULL)
    {
        if (last_entry != USER_ADDR_NULL)
        {
            last_entry->next_entry = new_entry;
        }
        else
        {
            symbol_table->first_entry = new_entry;
        }
        
        strncpy(new_entry->symbol, name, SYMBOL_MAX_LENGTH);
        new_entry->type = type;
        if (type == SYMBOL_BYTE)
        {
            new_entry->byte_value = byte_value;
            new_entry->byte_value_valid = 1;
            new_entry->word_value_valid = 0;
        }
        else
        {
            new_entry->word_value = word_value;
            new_entry->byte_value_valid = 0;
            new_entry->word_value_valid = 1;
        }
        new_entry->signedness = signedness;
        new_entry->next_entry = 0;

        symbol_reference_t *current_ref = symbol_table->first_reference;
        symbol_reference_t *next_ref;
        while (current_ref != USER_ADDR_NULL)
        {
            next_ref = current_ref;
            if (strncasecmp(current_ref->symbol, new_entry->symbol, SYMBOL_MAX_LENGTH) == 0)
            {
                current_ref->resolution = SYMBOL_ASSIGNED;
            }
            current_ref = next_ref;
        };

        return SYMBOL_ERROR_NOERROR;
    }
    else
    {
        return SYMBOL_ERROR_ALLOC_FAILED;
    }
}

symbol_ref_status_t add_symbol_reference(symbol_table_t *symbol_table, const char *name, uint16_t reference_address, symbol_signedness_t expected_signedness, symbol_type_t expected_type)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry;
    while (current_entry != USER_ADDR_NULL)
    {
        next_entry = current_entry->next_entry;
        
        if (strncasecmp(name, current_entry->symbol, SYMBOL_MAX_LENGTH) == 0)
        {
            break;
        }
        
        current_entry = next_entry;
    }

    if (current_entry != USER_ADDR_NULL)
    {
        if (current_entry->signedness != SIGNEDNESS_ANY && expected_signedness != SIGNEDNESS_ANY)
        {
            if (current_entry->signedness != expected_signedness)
            {
                return SYMBOL_REFERENCE_WRONG_SIGNEDNESS;
            }
        }

        if (current_entry->type != expected_type)
        {
            return SYMBOL_REFERENCE_WRONG_TYPE;
        }
    }

    symbol_reference_t *new_ref = malloc(sizeof(symbol_reference_t));
    if (new_ref == USER_ADDR_NULL)
    {
        return SYMBOL_REFERENCE_ALLOC_FAILED;
    }

    new_ref->next_reference = 0;
    new_ref->expected_signedness = expected_signedness;
    new_ref->expected_type = expected_type;
    new_ref->location = reference_address;
    new_ref->resolution = SYMBOL_UNASSIGNED;
    strncpy(new_ref->symbol, name, SYMBOL_MAX_LENGTH);

    symbol_reference_t *current_ref = symbol_table->first_reference;

    if (current_ref == USER_ADDR_NULL)
    {
        symbol_table->first_reference = new_ref;
    }
    else
    {
        symbol_reference_t *next_ref = 0;
        symbol_reference_t *last_ref = 0;
        while (current_ref->next_reference != USER_ADDR_NULL)
        {
            next_ref = current_ref->next_reference;
            last_ref = current_ref;
            current_ref = next_ref;
        }

        last_ref->next_reference = new_ref;
    }
 
    if (current_entry != USER_ADDR_NULL)
    {
        return SYMBOL_REFERENCE_RESOLVABLE;
    }

    return SYMBOL_REFERENCE_SUCCESS;
}

symbol_table_entry_t *get_symbol(symbol_table_t *symbol_table, const char *name)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry;
    while (current_entry != USER_ADDR_NULL)
    {
        next_entry = current_entry->next_entry;
        if (strncasecmp(current_entry->symbol, name, SYMBOL_MAX_LENGTH) == 0)
        {
            return current_entry;
        }
        current_entry = next_entry;
    }

    return 0;
}

symbol_resolution_t resolve_symbol(symbol_table_t *symbol_table, const char *name, symbol_type_t *symbol_type, symbol_signedness_t *signedness, uint16_t *word_value, uint8_t *byte_value)
{
    symbol_table_entry_t *entry = get_symbol(symbol_table, name);
    if (entry == USER_ADDR_NULL)
    {
        return SYMBOL_UNASSIGNED;
    }

    *symbol_type = entry->type;
    *signedness = entry->signedness;
    *word_value = entry->word_value;
    *byte_value = entry->byte_value;

    return SYMBOL_ASSIGNED;
}

symbol_ref_status_t check_all_symbols_resolved(symbol_table_t *symbol_table, int *unresolved_name_count, char **unresolved_symbol_names)
{
    int unresolved_count = 0;
    symbol_reference_t *current_ref = symbol_table->first_reference;
    symbol_reference_t *next_ref;
    while (current_ref != USER_ADDR_NULL)
    {
        next_ref = current_ref;
        if (current_ref->resolution == SYMBOL_UNASSIGNED)
        {
            strncpy(unresolved_symbol_names[unresolved_count], current_ref->symbol, SYMBOL_MAX_LENGTH);
            unresolved_count++;
        }
        current_ref = next_ref;
        if (unresolved_count >= *unresolved_name_count)
        {
            return SYMBOL_REFERENCE_NAMES_EXCEEDED;
        }
    };

    if (unresolved_count == 0)
    {
        return SYMBOL_REFERENCE_RESOLVABLE;
    }

    *unresolved_name_count = unresolved_count;

    return SYMBOL_REFERENCE_UNRESOLVED;
}
