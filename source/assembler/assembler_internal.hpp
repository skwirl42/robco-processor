#pragma once

#include <string>

#include <boost/exception/all.hpp>

#include "assembler.hpp"
#include "parser_types.hpp"

struct parser_error_message
{
    std::string message;
    std::string file;
    int current_line;
    parser_state current_state;
    parser_state expected_state;
};

typedef boost::error_info<struct tag_assembler_error, parser_error_message> parser_error;

void add_file_to_process(assembler_data_t *data, const char *included_file);
void handle_file(assembler_data_t *data, const char *included_file);
void handle_symbol_def(assembler_data_t *data, const char *name, int value, symbol_type_t type);
void handle_org_directive(assembler_data_t *data, uint16_t address);
void handle_instruction(assembler_data_t *data, const opcode_entry_t *opcode, const char *symbol_arg, int literal_arg);
void handle_indexed_instruction(assembler_data_t *data, const opcode_entry_t *opcode, const register_index_t &index_register);
void add_data(assembler_data_t* data, const std::string& name, const rc_assembler::byte_array& bytes);
void reserve_data(assembler_data_t* data, const char* name, uint16_t size);
void add_error(assembler_data_t *data, const char *error_string, assembler_status status, const char* filename = nullptr);
void add_error(assembler_data_t* data, std::string& error_string, assembler_status status, const char* filename = nullptr);
std::string current_filename(assembler_data_t* data);
