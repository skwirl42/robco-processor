#include "assembler_internal.h"

#include <errno.h>
#include <optional>
#include <filesystem>
#include "opcodes.h"
#include "memory.h"
#include "executable_file.h"
#include "assembler_parser.hpp"

namespace
{
    const uint16_t MIN_INSTRUCTION_ALLOC_SIZE = 0x100;
}

typedef struct _assembler_data
{
    const char **search_paths = nullptr;
    std::vector<assembled_region_t*> regions{};
    assembled_region_t *current_region = nullptr;
    symbol_table_t *symbol_table = nullptr;
    int lineNumber = 0;
    const char *current_filename = nullptr;
    std::vector<char*> files_to_process{};
    std::vector<assembler_error_t> errors{};
    char * error_buffer = nullptr;
    int error_buffer_size = 0;
    int symbol_references_count = 0;
    uint16_t current_org_address = 0;
    std::optional<uint16_t> execution_start{};
    bool current_org_address_valid = false;
    std::string output_filename{};
} assembler_data_t;

char *error_buffer = new char[ERROR_BUFFER_SIZE + 1];
char temp_buffer[ERROR_BUFFER_SIZE + 1];
assembler_data_t *assembler_data;
const char *current_filename;
int *lineNumber;

uint16_t executable_file_size(assembler_data_t *data)
{
    uint16_t file_size = sizeof(executable_file_header_t);
    for (auto segment : data->regions)
    {
        auto data_length = segment->executable ? segment->current_instruction_offset : segment->data_length;
        if (data_length > 0)
        {
            file_size += EXEC_SEGMENT_HEADER_RAW_SIZE + data_length;
        }
    }

    if (file_size == sizeof(executable_file_header_t))
    {
        return 0;
    }

    return file_size;
}

assembler_status_t prepare_executable_file(assembler_data_t *data, uint8_t *buffer)
{
    uint16_t file_size = executable_file_size(data);
    std::vector<assembled_region_t*> included_segments;
    for (auto segment : data->regions)
    {
        auto data_length = segment->executable ? segment->current_instruction_offset : segment->data_length;
        if (data_length > 0)
        {
            included_segments.push_back(segment);
        }
    }

    if (file_size == 0 || included_segments.size() == 0)
    {
        return ASSEMBLER_NOOUTPUT;
    }

    uint16_t buffer_index = 0;
    uint16_t segment_header_size = EXEC_SEGMENT_HEADER_RAW_SIZE;
    executable_file_header_t header = { file_size, (uint16_t)included_segments.size(), data->execution_start.value() };
    executable_segment_header_t segment_header{};
    memcpy(&buffer[buffer_index], &header, sizeof(executable_file_header_t));
    buffer_index += sizeof(executable_file_header_t);

    for (auto segment : included_segments)
    {
        auto data_length = segment->executable ? segment->current_instruction_offset : segment->data_length;
        segment_header.segment_location = segment->start_location;
        segment_header.segment_length = segment_header_size + data_length;
        segment_header.is_code = segment->executable;
        memcpy(&buffer[buffer_index], &segment_header, segment_header_size);
        buffer_index += segment_header_size;
        memcpy(&buffer[buffer_index], segment->data, data_length);
        buffer_index += data_length;
    }

    //printf("\n");
    //for (int i = 0; i < buffer_index; i++)
    //{
    //    printf("0x%02x ", (int)buffer[i]);
    //}

    //printf("\n");

    return ASSEMBLER_SUCCESS;
}

assembler_status_t get_starting_executable_address(assembler_data_t *data, uint16_t *address)
{
    *address = 0;
    if (data->execution_start.has_value())
    {
        *address = data->execution_start.value();
        return ASSEMBLER_SUCCESS;
    }

    return ASSEMBLER_UNINITIALIZED_VALUE;
}

int get_error_buffer_size(assembler_data_t *data)
{
    return data->error_buffer_size;
}

const char *get_error_buffer(assembler_data_t *data)
{
    return data->error_buffer;
}

const char *get_output_filename(assembler_data_t *data)
{
    return data->output_filename.c_str();
}

bool region_contains_address(assembled_region_t *region, uint16_t address)
{
    auto next_region_address = region->start_location + region->length;
    return (address >= region->start_location) && (address < next_region_address);
}

bool region_intersects_range(assembled_region_t *region, uint16_t address, uint16_t length)
{
    auto query_end_address = address + length - 1;
    return region_contains_address(region, address) || region_contains_address(region, query_end_address);
}

assembled_region_t *find_region_containing(assembler_data_t *data, uint16_t address)
{
    for (auto region : data->regions)
    {
        if (region_contains_address(region, address))
        {
            return region;
        }
    }

    return nullptr;
}

int find_new_address_for_region_of_size(assembler_data_t *data, uint16_t size)
{
    uint16_t search_start = 0x100;
    bool found_new_start = false;

    if (data->regions.size() > 0)
    {
        for (auto region : data->regions)
        {
            if (region_intersects_range(region, search_start, size))
            {
                search_start = region->start_location + region->length;
                found_new_start = true;
            }
            else
            {
                return search_start;
            }
        }

        // We've exhausted the regions, but we didn't do anything with the final
        // search position
        if (found_new_start && (search_start + size) <= DATA_SIZE)
        {
            return search_start;
        }
    }
    else
    {
        return search_start;
    }

    return -1;
}

assembled_region_t *create_new_region(assembler_data_t *data, uint16_t size, bool allocate_memory = true, int base_address = -1)
{
    assembled_region_t *new_region = nullptr;
    if (base_address >= 0)
    {
        bool region_available = true;
        for (auto region : data->regions)
        {
            if (region_intersects_range(region, base_address, size))
            {
                region_available = false;
                break;
            }
        }

        if (region_available)
        {
            new_region = new assembled_region_t { (uint16_t)base_address, 0, size };
            new_region->data = nullptr;
            new_region->data_length = 0;
            new_region->length = size;

            if (allocate_memory)
            {
                new_region->data = new uint8_t[size];
                new_region->data_length = size;
            }
        }
        else
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "No space to place a region at 0x%04x of %d size", base_address, size);
            add_error(data, temp_buffer, ASSEMBLER_NO_FREE_ADDRESS_RANGE);
        }
    }
    else
    {
        // Find an empty starting address that can fit this data
        int new_address = find_new_address_for_region_of_size(data, size);
        if (new_address >= 0)
        {
            new_region = new assembled_region_t { (uint16_t)new_address, 0, size };
            new_region->data = nullptr;
            new_region->data_length = 0;
            new_region->length = size;

            if (allocate_memory)
            {
                new_region->data = new uint8_t[size];
                new_region->data_length = size;
            }
        }
        else
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Couldn't find a suitable address to place a region of %d size", size);
            add_error(data, temp_buffer, ASSEMBLER_NO_FREE_ADDRESS_RANGE);
        }
    }

    if (new_region == nullptr)
    {
        if (base_address >= 0)
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "No available space for data at %d of size %d", base_address, size);
        }
        else
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "No available space for data of size %d", size);
        }
        
        add_error(data, temp_buffer, ASSEMBLER_NO_FREE_ADDRESS_RANGE);
    }
    else
    {
        // By default regions are not marked as executable.
        // Functions adding code should set their regions to
        // executable
        new_region->executable = false;
        data->regions.push_back(new_region);
    }
    
    return new_region;
}

// Returns the amount the region has been extended by
// This value can be smaller than extend_by, and must be checked by the caller
// By default the extension will be by MIN_INSTRUCTION_ALLOC_SIZE, since it's likely to only
//  be used for instructions as data definitions
uint16_t extend_region(assembler_data_t *data, assembled_region_t *region, uint16_t extend_by = MIN_INSTRUCTION_ALLOC_SIZE)
{
    uint16_t extended_by = 0;
    bool error = false;
    auto new_size = region->length + extend_by;
    auto extend_it = [region](uint16_t new_size)
    {
        auto current_data = region->data;
        auto data_length = region->data_length;
        auto new_region = new uint8_t[new_size];
        memcpy(new_region, current_data, data_length);
        region->length = new_size;
        region->data_length = new_size;
    };

    for (auto existing_region : data->regions)
    {
        if (existing_region != region && region_intersects_range(existing_region, region->start_location, new_size))
        {
            // We'd extend into another region, so we'll need to reduce the number of bytes we're allocating
            auto available_size = existing_region->start_location - region->start_location;
            if (available_size < region->length)
            {
                error = true;
                // TODO: Handle this overlap error!
            }
            else if (available_size == region->length)
            {
                error = true;
                // TODO: Report that there's no room to grow
            }
            else
            {
                auto region_length = region->length;
                extend_it(available_size);
                extended_by = available_size - region_length;
                break;
            }
        }
    }

    if (!error && extended_by == 0)
    {
        auto region_length = region->length;
        extend_it(new_size);
        extended_by = new_size - region_length;
    }

    return extended_by;
}

byte_array_t add_to_byte_array(byte_array_t array, uint8_t value)
{
    auto old_array = array.array;
    auto old_size = array.size;
    array.array = new uint8_t[(size_t)array.size + 1];
    memcpy(array.array, old_array, array.size);
    array.array[array.size++] = value;
    if (old_size > 0 && old_array != nullptr)
    {
        delete [] old_array;
    }
    return array;
}

void add_file_to_process(assembler_data_t *data, const char *filename)
{
    char *new_file = new char[LINE_BUFFER_SIZE + 1];
    strncpy(new_file, filename, LINE_BUFFER_SIZE);
    data->files_to_process.push_back(new_file);
}

void add_data(assembler_data_t *data, const char *name, byte_array_t array)
{
    assembled_region_t *region = create_new_region(data, array.size, true);

    if (region == nullptr)
    {
        return;
    }

    if (name != nullptr)
    {
        handle_symbol_def(data, name, region->start_location, SYMBOL_ADDRESS_DATA);
    }

    memcpy(region->data, array.array, array.size);

    array.size = 0;
    delete [] array.array;
    array.array = nullptr;
}

void add_string_to_data(assembler_data_t *data, const char *name, const char *string)
{
    uint16_t string_size = strlen(string);
    assembled_region_t *region = create_new_region(data, string_size + 1, true);

    if (region == nullptr)
    {
        return;
    }

    if (name != nullptr)
    {
        handle_symbol_def(data, name, region->start_location, SYMBOL_ADDRESS_DATA);
    }

    memcpy(region->data, string, strlen(string));
    region->data[strlen(string)] = 0;
}

void reserve_data(assembler_data_t* data, const char* name, uint16_t size)
{
    assembled_region_t *region = create_new_region(data, size, false);
    handle_symbol_def(data, name, region->start_location, SYMBOL_ADDRESS_DATA);
}

void handle_file(assembler_data_t *data, const char *filename)
{
    auto old_filename = data->current_filename;
    auto old_line_number = data->lineNumber;
    data->current_filename = filename;

    FILE *file = fopen(filename, "r");

    if (file == 0 && data->search_paths != 0)
    {
        const char * current_search_path = 0;
        int i = 0;
        while ((current_search_path = data->search_paths[i++]) != 0)
        {
            std::filesystem::path search_path{ current_search_path };
            search_path /= filename;
            file = fopen(search_path.string().c_str(), "r");
            if (file != 0)
            {
                break;
            }
        }
    }

    if (file != 0)
    {
        // printf("Processing %s\n", filename);
        int lineNumber = 1;
        char lineBuffer[LINE_BUFFER_SIZE + 1];
        int charIndex = 0;
        int currentChar;
        bool had_eof = false;
        while (!had_eof && (currentChar = fgetc(file)))
        {
            if (currentChar == EOF)
            {
                had_eof = true;
            }

            if (currentChar == '\r' || currentChar == '\n' || currentChar == EOF)
            {
                data->lineNumber = lineNumber;
                lineBuffer[charIndex] = 0;
                // Process the line
                if (charIndex > 0)
                {
                    parse_line(lineBuffer);
                    // fprintf(stdout, "Got line %d: %s\n", lineNumber, lineBuffer);
                }

                if (data->files_to_process.size() > 0)
                {
                    auto it = data->files_to_process.begin();
                    while (it != data->files_to_process.end())
                    {
                        auto file = *it;
                        data->files_to_process.erase(data->files_to_process.begin());
                        it = data->files_to_process.begin();
                        handle_file(data, file);
                        delete [] file;
                    }
                }

                charIndex = 0;
                lineNumber++;
                continue;
            }

            if (charIndex >= LINE_BUFFER_SIZE)
            {
                // Deal with this case
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Line %d is too long", lineNumber);
                add_error(data, temp_buffer, ASSEMBLER_INPUT_ERROR);
            }
            else
            {
                lineBuffer[charIndex++] = currentChar;
            }
        }

        if (currentChar == EOF && ferror(file) != 0)
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "File read error (%s - %d)", filename, ferror(file));
            add_error(data, temp_buffer, ASSEMBLER_IO_ERROR);
        }

        fclose(file);
    }
    else
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Failed to open file (%s)", filename);
        fprintf(stderr, "%s\n", temp_buffer);
        add_error(data, temp_buffer, ASSEMBLER_IO_ERROR);
    }

    data->lineNumber = old_line_number;
    data->current_filename = old_filename;
}

int get_current_instruction_address(assembler_data_t *data)
{
    // TODO:
    // If not enough room for the post-bytes in the current region, then allocate a new region for them
    // OR
    // Extend the region, if possible
    if (data->current_region != nullptr)
    {
        uint16_t address = data->current_region->start_location + data->current_region->current_instruction_offset;
        if (region_contains_address(data->current_region, address) && region_contains_address(data->current_region, address + 2))
        {
            if (data->current_region->executable)
            {
                return data->current_region->start_location + data->current_region->current_instruction_offset;
            }
            else
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Memory at address 0x%04x is not executable - overlaps with data region starting at 0x%04x", address, data->current_region->start_location);
                add_error(data, temp_buffer, ASSEMBLER_INTERNAL_ERROR);
                return -1;
            }
        }
        else if (region_contains_address(data->current_region, address) && !region_contains_address(data->current_region, address + 2))
        {
            auto extended_by_bytes = extend_region(data, data->current_region);
            if (extended_by_bytes < 2)
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Couldn't extend region at 0x%04x to accommodate new instructions", address);
                add_error(data, temp_buffer, ASSEMBLER_NO_FREE_ADDRESS_RANGE);
                return -1;
            }
            else
            {
                return address;
            }
        }
        else
        {
            // TODO: See if there's already an allocation for it,
            // OR
            // Create a new region
        }
    }
    else
    {
        auto new_region = create_new_region(data, MIN_INSTRUCTION_ALLOC_SIZE, true);
        if (new_region != nullptr)
        {
            new_region->executable = true;
            data->current_region = new_region;
            return new_region->start_location;
        }
    }
    return -1;
}

void symbol_resolution_callback(void *context, uint16_t ref_location, symbol_type_t symbol_type, symbol_signedness_t expected_signedness, uint8_t byte_value, machine_word_t word_value)
{
    auto data = reinterpret_cast<assembler_data_t*>(context);
    auto region = find_region_containing(data, ref_location);
    if (region == nullptr)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Failed to find a region containing a symbol reference (should be at 0x%04x)", ref_location);
        add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
        return;
    }

    auto ref_index = ref_location - region->start_location;
    if (symbol_type == SYMBOL_BYTE)
    {
        region->data[ref_index] = byte_value;
    }
    else if (symbol_type == SYMBOL_ADDRESS_INST && expected_signedness == SIGNEDNESS_SIGNED)
    {
        auto address_offset = (int)word_value.uword - ((int)ref_location - 1);
        if (address_offset > 127 || address_offset < -128)
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Tried to branch too far (from 0x%04x to 0x%04x)", ref_location, word_value.uword);
            add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
            return;
        }
        else
        {
            auto signed_byte = (int8_t)address_offset;
            auto unsigned_byte = *((uint8_t*)&signed_byte);
            region->data[ref_index] = unsigned_byte;
        }
    }
    else
    {
        region->data[ref_index] = word_value.bytes[1];
        region->data[ref_index + 1] = word_value.bytes[0];
    }
}

void handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type)
{
    symbol_error_t sym_err = SYMBOL_ERROR_NOERROR;
    if ((type == SYMBOL_ADDRESS_INST || type == SYMBOL_ADDRESS_DATA) && value < 0)
    {
        sym_err = SYMBOL_ERROR_INTERNAL;
    }

    if (type == SYMBOL_ADDRESS_INST && value == 0)
    {
        value = get_current_instruction_address(data);
    }

    if (sym_err == SYMBOL_ERROR_NOERROR)
    {
    switch (type)
    {
        case SYMBOL_WORD:
        case SYMBOL_ADDRESS_INST:
        case SYMBOL_ADDRESS_DATA:
            sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, value, 0, true);
            break;

        case SYMBOL_BYTE:
            sym_err = add_symbol(data->symbol_table, name, type, SIGNEDNESS_ANY, 0, value, true);
            break;

        case SYMBOL_NO_TYPE:
            sym_err = SYMBOL_ERROR_INTERNAL;
            break;
        }
    }

    if (sym_err == SYMBOL_ERROR_ALLOC_FAILED)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Couldn't allocate space for a symbol named %s", name);
        add_error(data, temp_buffer, ASSEMBLER_ALLOC_FAILED);
    }
    else if (sym_err == SYMBOL_ERROR_EXISTS)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Tried to redefine a symbol named %s", name);
        add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
    }
    else if (sym_err == SYMBOL_ERROR_INTERNAL)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Internal error trying to define a symbol named %s", name);
        add_error(data, temp_buffer, ASSEMBLER_INTERNAL_ERROR);
    }
}

void handle_org_directive(assembler_data_t *data, uint16_t address)
{
    assembled_region_t *target_region = nullptr;
    for (auto region : data->regions)
    {
        if (region_contains_address(region, address))
        {
            target_region = region;
        }
    }

    if (target_region == nullptr)
    {
        target_region = create_new_region(data, MIN_INSTRUCTION_ALLOC_SIZE, true, address);
    }

    if (target_region != nullptr)
    {
        target_region->current_instruction_offset = address - target_region->start_location;
        target_region->executable = true;
        data->current_region = target_region;
    }
}

void apply_machine_instruction(assembler_data_t *data, uint8_t opcode, opcode_entry_t* opcode_entry = nullptr, std::optional<uint8_t> byte0 = std::nullopt, std::optional<uint8_t> byte1 = std::nullopt)
{
    auto address = get_current_instruction_address(data);
    if (!data->execution_start.has_value())
    {
        data->execution_start = address;
    }

    // Write the bytes to the current region
    auto offset = data->current_region->current_instruction_offset;

    data->current_region->data[offset++] = opcode;

    int byte_count = 0;
    if (byte0.has_value())
    {
        data->current_region->data[offset++] = byte0.value();
        byte_count++;
    }

    if (byte1.has_value())
    {
        data->current_region->data[offset++] = byte1.value();
        byte_count++;
    }

    data->current_region->current_instruction_offset = offset;

    if (opcode_entry != nullptr)
    {
        //printf("Assembling %s at 0x%04x with %d argument bytes\n", opcode_entry->name, address, byte_count);
    }
}

void handle_instruction(assembler_data_t *data, opcode_entry_t *opcode, const char *symbol_arg, int literal_arg)
{
    int current_instruction_address = get_current_instruction_address(data);
    if (current_instruction_address < 0 || data->current_region == nullptr)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Not able to locate or allocate new instruction memory");
        add_error(data, temp_buffer, ASSEMBLER_INTERNAL_ERROR);
        return;
    }

    auto current_region = data->current_region;

    // printf("Handling instruction %s with arg (%s)\n", opcode->name, symbol_arg ? symbol_arg : "numerical");
    if (opcode->access_mode == STACK_ONLY && (symbol_arg != nullptr || literal_arg != 0))
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Stack-only instruction %s cannot take parameters", opcode->name);
        add_error(data, temp_buffer, ASSEMBLER_INVALID_ARGUMENT);
    }
    else if (opcode->access_mode == REGISTER_INDEXED)
    {
        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Register indexed instruction %s cannot be handled through this function", opcode->name);
        add_error(data, temp_buffer, ASSEMBLER_INVALID_ARGUMENT);
    }
    else if (symbol_arg)
    {
        symbol_type_t type;
        symbol_signedness_t signedness = SIGNEDNESS_ANY;
        uint16_t word_value;
        uint8_t byte_value;
        auto resolution = resolve_symbol(data->symbol_table, symbol_arg, &type, &signedness, &word_value, &byte_value);
        if (resolution != SYMBOL_ASSIGNED)
        {
            symbol_ref_status_t add_ref_result;
            if (opcode->argument_type == SYMBOL_ADDRESS_INST)
            {
                if (IS_BRANCH_INST(opcode->opcode))
                {
                    signedness = SIGNEDNESS_SIGNED;
                }
                add_ref_result = add_symbol_reference(data->symbol_table, symbol_arg, 
                                                        symbol_resolution_callback, data, current_instruction_address + 1,
                                                        signedness, SYMBOL_ADDRESS_INST);
            }
            else 
            {
                add_ref_result = add_symbol_reference(data->symbol_table, symbol_arg,
                                                        symbol_resolution_callback, data, current_instruction_address + 1,
                                                        opcode->argument_signedness, opcode->argument_type);
            }

            if (add_ref_result != SYMBOL_REFERENCE_RESOLVABLE && add_ref_result != SYMBOL_REFERENCE_SUCCESS)
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Error trying to add a reference to symbol %s", symbol_arg);
                add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
            }
            data->symbol_references_count++;
        }
        machine_word_t word;
        word.uword = word_value;
        if (resolution == SYMBOL_ASSIGNED)
        {
            switch (type)
            {
            case SYMBOL_WORD:
            case SYMBOL_ADDRESS_DATA:
                apply_machine_instruction(data, opcode->opcode, opcode, word.bytes[1], word.bytes[0]);
                break;

            case SYMBOL_BYTE:
                apply_machine_instruction(data, opcode->opcode, opcode, byte_value);
                break;

            case SYMBOL_ADDRESS_INST:
                if (opcode->argument_signedness == SIGNEDNESS_SIGNED && opcode->arg_byte_count == 1)
                {
                    auto address_offset = (int)word_value - current_instruction_address;
                    if (address_offset > 127 || address_offset < -128)
                    {
                        snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Tried to branch too far (from 0x%04x to 0x%04x)", current_instruction_address, word_value);
                        add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
                        return;
                    }
                    else
                    {
                        auto signed_byte = (int8_t)address_offset;
                        auto unsigned_byte = *((uint8_t*)&signed_byte);
                        apply_machine_instruction(data, opcode->opcode, opcode, unsigned_byte);
                    }
                }
                else
                {
                    apply_machine_instruction(data, opcode->opcode, opcode, word.bytes[1], word.bytes[0]);
                }
                break;

            case SYMBOL_NO_TYPE:
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Error resolving symbol %s", symbol_arg);
                add_error(data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
                break;
            }
        }
        else if (opcode->argument_type == SYMBOL_ADDRESS_INST)
        {
            // The following byte(s) will be resolved later
            if (opcode->arg_byte_count == 1)
            {
                apply_machine_instruction(data, opcode->opcode, opcode, 0);
            }
            else if (opcode->arg_byte_count == 2)
            {
                apply_machine_instruction(data, opcode->opcode, opcode, 0, 0);
            }
            else
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Error handling opcode %s, which references %s", opcode->name, symbol_arg);
                add_error(data, temp_buffer, ASSEMBLER_INTERNAL_ERROR);
            }
        }
    }
    else if (opcode->argument_type == SYMBOL_NO_TYPE)
    {
        // There may be register specifications in the literal arg
        if (opcode->arg_byte_count == 0)
        {
            apply_machine_instruction(data, opcode->opcode, opcode);
        }
        else if (opcode->arg_byte_count == 1)
        {
            apply_machine_instruction(data, opcode->opcode, opcode, (uint8_t)literal_arg);
        }
        else if (opcode->arg_byte_count > 1)
        {
            apply_machine_instruction(data, opcode->opcode, opcode, 0, (uint8_t)literal_arg);
        }
    }
    else
    {
        // handle literal
        // Bounds check
        if ((opcode->argument_type == SYMBOL_WORD && (literal_arg > 65535 || literal_arg < -32768))
            || (opcode->argument_type == SYMBOL_BYTE && (literal_arg > 255 || literal_arg < -128)))
        {
            snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Opcode %s cannot accommodate a value of %d", opcode->name, literal_arg);
            add_error(data, temp_buffer, ASSEMBLER_VALUE_OOB);
        }
        else if (opcode->argument_type == SYMBOL_WORD)
        {
            if (literal_arg < 0)
            {
                literal_arg += 65536;
            }

            uint16_t literal = literal_arg;
            machine_word_t word;
            word.uword = literal;

            apply_machine_instruction(data, opcode->opcode, opcode, word.bytes[1], word.bytes[0]);
        }
        else
        {
            if (literal_arg < 0)
            {
                literal_arg += 256;
            }

            apply_machine_instruction(data, opcode->opcode, opcode, (uint8_t)literal_arg);
        }
    }
}

void handle_indexed_instruction(assembler_data_t *data, opcode_entry_t *opcode, register_index_t &index_register)
{
    // Prepare opcode
    uint8_t opcode_value = opcode->opcode | index_register.index_register->code;

    // Prepare increment post-byte
    int8_t increment_byte = index_register.increment_amount;
    if (index_register.is_pre_increment)
    {
        increment_byte |= OP_STACK_INCREMENT_PRE;
    }
    else
    {
        increment_byte &= ~OP_STACK_INCREMENT_PRE;
    }

    apply_machine_instruction(data, opcode_value, opcode, increment_byte);
}

void add_error(assembler_data_t *data, const char *error_string, assembler_status_t status)
{
    assembler_error_t error;
    error.status = status;
    error.line_number = data->lineNumber;
    error.error_start = data->error_buffer_size;
    int length = snprintf(&data->error_buffer[error.error_start], (size_t)ERROR_BUFFER_SIZE - data->error_buffer_size, "%s:%d - %s\n", data->current_filename, data->lineNumber, error_string);
    data->errors.push_back(error);
    data->error_buffer_size += length;
}

assembler_status_t apply_assembled_data_to_buffer(assembler_data_t *data, uint8_t *buffer)
{
    if (data->regions.size() == 0)
    {
        return ASSEMBLER_NOOUTPUT;
    }

    if (!data->execution_start.has_value())
    {
        return ASSEMBLER_UNINITIALIZED_VALUE;
    }

    for (auto region : data->regions)
    {
        uint8_t *region_start = &buffer[region->start_location];
        memcpy(region_start, region->data, region->data_length);
    }

    return ASSEMBLER_SUCCESS;
}

assembler_data_t data;
void assemble(const char *filename, const char **search_paths, const char *output_file, OutputFileType outFileType, assembler_data_t **assembled_data)
{
    data.search_paths = search_paths;
    data.lineNumber = 0;
    data.symbol_references_count = 0;
    data.current_org_address_valid = false;

    assembler_data = &data;

    error_buffer[0] = 0;
    data.error_buffer = error_buffer;
    data.error_buffer_size = 0;
    *assembled_data = assembler_data;

    current_filename = assembler_data->current_filename;
    lineNumber = &assembler_data->lineNumber;

    if (create_symbol_table(&data.symbol_table) != SYMBOL_TABLE_NOERROR)
    {
        add_error(&data, "Failed to create the symbol table", ASSEMBLER_ALLOC_FAILED);
        return;
    }

    handle_file(&data, filename);

    if (data.symbol_references_count > 0)
    {
        data.current_filename = filename;
        char **symbol_buffers = new char*[data.symbol_references_count];

        for (int i = 0; i < data.symbol_references_count; i++)
        {
            symbol_buffers[i] = new char[SYMBOL_MAX_LENGTH + 1];
        }

        int symbol_count = data.symbol_references_count;
        auto result = check_all_symbols_resolved(data.symbol_table, &symbol_count, symbol_buffers);

        if (result != SYMBOL_REFERENCE_RESOLVABLE)
        {
            for (int i = 0; i < symbol_count; i++)
            {
                snprintf(temp_buffer, ERROR_BUFFER_SIZE, "Unresolved symbol %s", symbol_buffers[i]);
                add_error(&data, temp_buffer, ASSEMBLER_SYMBOL_ERROR);
            }
        }

        for (int i = 0; i < data.symbol_references_count; i++)
        {
            delete [] symbol_buffers[i];
        }

        delete [] symbol_buffers;
    }

    if (data.errors.size() > 0)
    {
        return;
    }

    if (outFileType != None && outFileType != Error)
    {
        std::string output_filename{};
        if (output_file == nullptr)
        {
            std::filesystem::path infilepath(filename);
            switch (outFileType)
            {
            case Binary:
                output_filename = infilepath.stem().string() + ".rcexe";
                break;

            case Summary:
            default:
                output_filename = infilepath.stem().string() + ".txt";
                break;
            }
        }
        else
        {
            output_filename = output_file;
        }
        
        FILE* assembled_output = 0;
#if defined(_MSC_VER)
        assembled_output = fopen(output_filename.c_str(), "wb+");
#else
        assembled_output = fopen(output_filename.c_str(), "w+");
#endif
        if (assembled_output != 0)
        {
            if (outFileType == Summary)
            {    
                fprintf(assembled_output, "Execution start: 0x%04x\n", data.execution_start.value());

                fprintf(assembled_output, "Code:\n");
                int i = 0;
                auto output_line = [assembled_output, &i](assembled_region_t *region, int data_length)
                {
                    fprintf(assembled_output, "0x%04x: ", region->start_location + i);
                    for (int j = 0; j < 4 && i < region->data_length; j++)
                    {
                        fprintf(assembled_output, "0x%02x ", region->data[i++]);
                    }
                    fprintf(assembled_output, "\n");
                };

                for (auto region : data.regions)
                {
                    if (region->executable)
                    {
                        for (i = 0; i < region->data_length && i < region->current_instruction_offset;)
                        {
                            output_line(region, region->data_length);
                        }
                    }
                }

                const int max_data_print = 512;
                fprintf(assembled_output, "\nData:\n");
                for (auto region : data.regions)
                {
                    if (!region->executable)
                    {
                        auto data_length = region->data_length < max_data_print ? region->data_length : max_data_print;
                        if (region->data == nullptr || region->data_length == 0)
                        {
                            fprintf(assembled_output, "0x%04x: reserved %d bytes\n", region->start_location, region->length);
                        }
                        else
                        {
                            for (i = 0; i < data_length;)
                            {
                                output_line(region, data_length);
                            }

                            if (data_length != region->data_length)
                            {
                                fprintf(assembled_output, " ... \n");
                            }
                        }
                    }
                }

                fprintf(assembled_output, "\nSymbols:\n");
                output_symbols(assembled_output, data.symbol_table);
            }
            else if (outFileType == Binary)
            {
                auto file_size = executable_file_size(&data);
                std::unique_ptr<uint8_t[]> file_buffer(new uint8_t[file_size]);
                auto result = prepare_executable_file(&data, file_buffer.get());
                if (result == ASSEMBLER_SUCCESS)
                {
                    fwrite(file_buffer.get(), 1, file_size, assembled_output);
                }
                else
                {
                    fprintf(stderr, "Couldn't output assembled data to file %s\n", output_filename.c_str());
                }
            }

            fclose(assembled_output);
            data.output_filename = output_filename;
        }
        else
        {
            fprintf(stderr, "Couldn't open assembler output file %s (errno: %d)\n", output_filename.c_str(), errno);
        }
    }

    if (data.symbol_table != 0)
    {
        dispose_symbol_table(data.symbol_table);
    }
}