#pragma once

#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <vector>
#include <string>
#include <map>

#include "parser_types.hpp"
#include "assembler_internal.hpp"
#include "exceptions.hpp"

class data_def_handler
{
public:
    data_def_handler(assembler_data_t* data);

    parser_state begin(parser_state state, const rc_assembler::data_def& begin_data);
    parser_state add(parser_state state, const rc_assembler::byte_array& bytes);
    parser_state end(parser_state state);
    bool is_in_progress() const { return in_progress; }
    bool can_end_without_end_statement() const{ return in_progress && !spans_multiple_lines; }

private:
    std::vector<char> data_bytes{};
    std::string name{};
    assembler_data_t* data;
    bool spans_multiple_lines = false;
    bool in_progress = false;
};

class struct_def_handler
{
public:
    struct_def_handler(assembler_data_t* data);

    parser_state begin(parser_state state, const rc_assembler::struct_def& begin_def);
    parser_state add(parser_state state, const rc_assembler::struct_member_def& member_def);
    parser_state end(parser_state state);
    bool is_in_progress() const { return in_progress; }

private:
    std::vector<rc_assembler::struct_member_def> members{};
    std::string name{};
    assembler_data_t* data;
    bool in_progress = false;
};

class instruction_argument_visitor : public boost::static_visitor<>
{
public:
    instruction_argument_visitor(const opcode_entry_t* opcode) : opcode(opcode) {}

    void operator()(const register_index_t& register_index)
    {
        handle_indexed_instruction(assembler_data, opcode, register_index);
    }

    void operator()(const uint8_t& value)
    {
        handle_instruction(assembler_data, opcode, nullptr, value);
    }

    void operator()(const uint16_t& value)
    {
        handle_instruction(assembler_data, opcode, nullptr, value);
    }

    void operator()(const int& value)
    {
        if (opcode->arg_byte_count == 1 && (value < -128 || value > 255))
        {
            // error! out of bounds
            return;
        }
        else if (opcode->arg_byte_count == 2 && (value < -32768 || value > 65535))
        {
            // error! out of bounds
            return;
        }

        handle_instruction(assembler_data, opcode, nullptr, value);
    }

    void operator()(const rc_assembler::symbol& symbol)
    {
        handle_instruction(assembler_data, opcode, symbol.c_str(), 0);
    }

private:
    const opcode_entry_t* opcode;
};

class assembly_line_visitor : public boost::static_visitor<>
{
public:
    assembly_line_visitor(assembler_data_t*data) : data_handler(data), struct_handler(data), data(data)
    {}

    ~assembly_line_visitor()
    {
        if (!verify_current_state(parser_state::normal))
        {
            // throw or something
        }
    }

    void operator()(const rc_assembler::instruction_line& instruction)
    {
        if (verify_current_state(parser_state::normal))
        {
            if (instruction.opcode != nullptr)
            {
                if (instruction.argument)
                {
                    if (instruction.opcode->arg_byte_count == 0)
                    {
                        // error! oh noes!
                    }
                    else
                    {
                        instruction_argument_visitor visitor(instruction.opcode);
                        boost::apply_visitor(visitor, instruction.argument.value());
                    }
                }
                else if (instruction.opcode->arg_byte_count > 0)
                {
                    // error! the instruction requires an argument, but none is provided!
                    return;
                }
                else
                {
                    handle_instruction(assembler_data, instruction.opcode, nullptr, 0);
                }
            }
            else
            {
                std::cout << "instruction without opcode?!" << std::endl;
            }
        }
    }

    void operator()(const rc_assembler::reservation& reservation)
    {
        if (verify_current_state(parser_state::normal))
        {
            reserve_data(assembler_data, reservation.symbol.c_str(), reservation.size);
        }
    }

    void operator()(const rc_assembler::byte_def& byte)
    {
        if (verify_current_state(parser_state::normal))
        {
            handle_symbol_def(assembler_data, byte.symbol.c_str(), byte.value, SYMBOL_BYTE);
        }
    }

    void operator()(const rc_assembler::word_def& word)
    {
        if (verify_current_state(parser_state::normal))
        {
            handle_symbol_def(assembler_data, word.symbol.c_str(), word.value, SYMBOL_WORD);
        }
    }

    void operator()(const rc_assembler::data_def& def)
    {
        if (verify_current_state(parser_state::normal))
        {
            auto new_state = data_handler.begin(current_state, def);
            if (new_state == parser_state::error)
            {

            }
            current_state = new_state;
        }
    }

    void operator()(const rc_assembler::byte_array& bytes)
    {
        if (verify_current_state(parser_state::data_def_in_progress))
        {
            auto new_state = data_handler.add(current_state, bytes);
            if (new_state == parser_state::error)
            {

            }
            current_state = new_state;
        }
    }

    void operator()(const rc_assembler::end_data_def& end_data)
    {
        if (verify_current_state(parser_state::data_def_in_progress))
        {
            auto new_state = data_handler.end(current_state);
            if (new_state == parser_state::error)
            {

            }
            current_state = new_state;
        }
    }

    void operator()(const rc_assembler::org_def& def)
    {
        if (verify_current_state(parser_state::normal))
        {
            handle_org_directive(assembler_data, def.location);
        }
    }

    void operator()(const rc_assembler::label_def& def)
    {
        if (verify_current_state(parser_state::normal))
        {
            handle_symbol_def(assembler_data, def.label_name.c_str(), 0, SYMBOL_ADDRESS_INST);
        }
    }

    void operator()(const rc_assembler::include_def& def)
    {
        if (verify_current_state(parser_state::normal))
        {
            std::string string(def.included_file.begin(), def.included_file.end());
            add_file_to_process(assembler_data, string.c_str());
        }
    }

    void operator()(const rc_assembler::struct_def& def)
    {
        if (verify_current_state(parser_state::normal))
        {
            auto new_state = struct_handler.begin(current_state, def);
            if (new_state == parser_state::error)
            {

            }
            current_state = new_state;
        }
    }

    void operator()(const rc_assembler::struct_member_def& def)
    {
        if (verify_current_state(parser_state::struct_def_in_progress))
        {
            auto new_state = struct_handler.add(current_state, def);
            if (new_state == parser_state::error)
            {

            }
            current_state = new_state;
        }
    }

    void operator()(const rc_assembler::end_struct_def& def)
    {
        if (verify_current_state(parser_state::struct_def_in_progress))
        {
            auto new_state = struct_handler.end(current_state);
            if (new_state == parser_state::error)
            {

            }
            current_state = new_state;
        }
    }

private:
    // Call from non-data handlers to verify that no data definitions are in progress
    bool verify_current_state(parser_state expected_state)
    {
        bool return_value = true;
        if ((expected_state != parser_state::data_def_in_progress 
            // The line below this is to handle two, single-line data def statements in a row
            || (expected_state == parser_state::data_def_in_progress && data_handler.can_end_without_end_statement()))
            && data_handler.is_in_progress())
        {
            if (data_handler.can_end_without_end_statement())
            {
                auto new_state = data_handler.end(current_state);
                if (new_state == parser_state::error)
                {

                }
                current_state = new_state;
                return_value = true;
            }
            else
            {
                return_value = false;
            }
        }
        else if (expected_state != parser_state::struct_def_in_progress && struct_handler.is_in_progress())
        {
            return_value = false;
        }
        else if (current_state != expected_state)
        {
            return_value = false;
        }

        if (!return_value)
        {
            parser_error_message error
            {
                std::string("Parser in the wrong state"),
                std::string(current_filename(data)),
                *lineNumber,
                current_state,
                expected_state
            };
            throw assembler_error() << parser_error(error);
        }

        return return_value;
    }

private:
    parser_state current_state = parser_state::normal;
    data_def_handler data_handler;
    struct_def_handler struct_handler;
    assembler_data_t* data;
};
