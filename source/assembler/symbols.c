#include "symbols.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "opcodes.h"

typedef struct _symbol_reference symbol_reference_t;
typedef struct _symbol_table_entry symbol_table_entry_t;

struct _symbol_reference
{
    char symbol[SYMBOL_MAX_LENGTH + 1];
    symbol_reference_t *next_reference;
    uint8_t *location;
    uint16_t ref_location;
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

void output_symbols(FILE *output_file, symbol_table_t *symbol_table)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    while (current_entry != 0)
    {
        if (current_entry->word_value_valid)
        {
            fprintf(output_file, "Name: %s, value: 0x%04x %s\n", current_entry->symbol, current_entry->word_value, (current_entry->type == SYMBOL_ADDRESS_INST) ? "(address)" : "");
        }
        else
        {
            fprintf(output_file, "Name: %s, value: 0x%02x\n", current_entry->symbol, current_entry->byte_value);
        }
        
        current_entry = current_entry->next_entry;
    }

    fprintf(output_file, "\nReferences:\n");
    symbol_reference_t *current_reference = symbol_table->first_reference;
    while (current_reference != 0)
    {
        fprintf(output_file, "Reference: %s, near location 0x%04x\n", current_reference->symbol, current_reference->ref_location);
        current_reference = current_reference->next_reference;
    }
}

symbol_table_error_t create_symbol_table(symbol_table_t **symbol_table)
{
    *symbol_table = malloc(sizeof(struct _symbol_table));
    if (symbol_table == 0)
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
    while (current_ref != 0)
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
    if (type == SYMBOL_NO_TYPE)
    {
        fprintf(stderr, "Trying to define a symbol (%s) with no type\n", name);
        return SYMBOL_ERROR_INTERNAL;
    }

    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry = 0;
    symbol_table_entry_t *last_entry = 0;
    while (current_entry != 0)
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
    if (new_entry != 0)
    {
        if (last_entry != 0)
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
        while (current_ref != 0)
        {
            if (strncasecmp(current_ref->symbol, new_entry->symbol, SYMBOL_MAX_LENGTH) == 0)
            {
                current_ref->resolution = SYMBOL_ASSIGNED;
                if (type == SYMBOL_BYTE)
                {
                    *current_ref->location = byte_value;
                }
                else if (current_ref->expected_signedness == SIGNEDNESS_ANY || current_ref->expected_signedness == SIGNEDNESS_UNSIGNED)
                {
                    machine_word_t word;
                    word.uword = word_value;
                    current_ref->location[0] = word.bytes[1];
                    current_ref->location[1] = word.bytes[0];
                }
                else if (current_ref->expected_signedness == SIGNEDNESS_SIGNED)
                {
                    machine_word_t word;
                    word.uword = word_value;
                    word.sword -= (int16_t)current_ref->ref_location - 1;
                    current_ref->location[0] = word.bytes[1];
                    current_ref->location[1] = word.bytes[0];
                }
            }
            current_ref = current_ref->next_reference;
        };

        // fprintf(stdout, "Creating new symbol \"%s\" with value %d\n", new_entry->symbol, new_entry->byte_value_valid ? new_entry->byte_value : new_entry->word_value);

        return SYMBOL_ERROR_NOERROR;
    }
    else
    {
        return SYMBOL_ERROR_ALLOC_FAILED;
    }
}

symbol_ref_status_t add_symbol_reference(symbol_table_t *symbol_table, const char *name, uint8_t *reference_address, uint16_t ref_location, symbol_signedness_t expected_signedness, symbol_type_t expected_type)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry;
    while (current_entry != 0)
    {
        next_entry = current_entry->next_entry;
        
        if (strncasecmp(name, current_entry->symbol, SYMBOL_MAX_LENGTH) == 0)
        {
            break;
        }
        
        current_entry = next_entry;
    }

    if (current_entry != 0)
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
    if (new_ref == 0)
    {
        return SYMBOL_REFERENCE_ALLOC_FAILED;
    }

    new_ref->next_reference = 0;
    new_ref->expected_signedness = expected_signedness;
    new_ref->expected_type = expected_type;
    new_ref->location = reference_address;
    new_ref->ref_location = ref_location;
    new_ref->resolution = SYMBOL_UNASSIGNED;
    strncpy(new_ref->symbol, name, SYMBOL_MAX_LENGTH);

    symbol_reference_t *current_ref = symbol_table->first_reference;

    if (current_ref == 0)
    {
        symbol_table->first_reference = new_ref;
    }
    else
    {
        symbol_reference_t *next_ref = 0;
        symbol_reference_t *last_ref = 0;
        while (current_ref->next_reference != 0)
        {
            next_ref = current_ref->next_reference;
            last_ref = current_ref;
            current_ref = next_ref;
        }

        last_ref->next_reference = new_ref;
    }
 
    if (current_entry != 0)
    {
        return SYMBOL_REFERENCE_RESOLVABLE;
    }

    return SYMBOL_REFERENCE_SUCCESS;
}

symbol_table_entry_t *get_symbol(symbol_table_t *symbol_table, const char *name)
{
    symbol_table_entry_t *current_entry = symbol_table->first_entry;
    symbol_table_entry_t *next_entry;
    while (current_entry != 0)
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
    if (entry == 0)
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
    while (current_ref != 0)
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
