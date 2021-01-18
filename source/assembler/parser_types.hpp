#pragma once

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/variant.hpp>

#include <optional>

#include "opcodes.h"

enum class parser_state
{
    normal,
    error,
    data_def_in_progress,
    struct_def_in_progress
};

struct register_index_t
{
    register_argument_t* index_register;
    uint8_t is_pre_increment;
    int8_t increment_amount;

    register_index_t() : index_register(0), is_pre_increment(0), increment_amount(0) {}
    register_index_t(const register_index_t& other)
        : index_register(other.index_register),
        is_pre_increment(other.is_pre_increment),
        increment_amount(other.increment_amount)
    {
    }
    register_index_t(register_argument_t* index_register, uint8_t is_pre_increment, int8_t increment_amount) :
        index_register(index_register), is_pre_increment(is_pre_increment), increment_amount(increment_amount)
    {

    }
};

namespace rc_assembler
{
    namespace fusion = boost::fusion;
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    typedef std::vector<char> byte_array;
    typedef std::string symbol;
    typedef boost::variant<register_index_t, uint8_t, uint16_t, int, symbol> instruction_argument;

    struct instruction_line
    {
        opcode_entry_t const* opcode = nullptr;
        std::optional<instruction_argument> argument{};
    };

    struct reservation
    {
        symbol symbol{};
        size_t size = 0;
    };

    struct include_def
    {
        byte_array included_file{};
    };

    struct label_def
    {
        symbol label_name{};
    };

    struct byte_def
    {
        symbol symbol{};
        uint8_t value = 0;
    };

    struct word_def
    {
        symbol symbol{};
        uint16_t value = 0;
    };

    struct data_def
    {
        std::optional<symbol> symbol{};
        byte_array bytes{};
    };

    struct end_data_def
    {
        std::string contents{};
    };

    struct org_def
    {
        uint16_t location = 0;
    };

    struct struct_def
    {
        symbol symbol{};
    };

    struct struct_member_def
    {
        symbol symbol{};
        uint16_t size = 0;
    };

    struct end_struct_def
    {
        std::string contents{};
    };;

    typedef boost::variant
    <
        instruction_line, 
        reservation, 
        byte_def, 
        byte_array,
        word_def,
        data_def,
        end_data_def,
        org_def,
        label_def,
        include_def,
        struct_def,
        struct_member_def,
        end_struct_def
    > line_options;
    
    struct assembly_line
    {
        std::optional<line_options> line_options;
        std::optional<std::string> comment;
    };
 } // namespace rc_assembler

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::instruction_line,
    (opcode_entry_t const*, opcode),
    (std::optional<rc_assembler::instruction_argument>, argument)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::reservation,
    (rc_assembler::symbol, symbol),
    (size_t, size)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::include_def,
    (rc_assembler::byte_array, included_file)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::byte_def,
    (rc_assembler::symbol, symbol),
    (uint8_t, value)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::word_def,
    (rc_assembler::symbol, symbol),
    (uint16_t, value)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::label_def,
    (rc_assembler::symbol, label_name)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::data_def,
    (std::optional<rc_assembler::symbol>, symbol),
    (rc_assembler::byte_array, bytes)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::end_data_def,
    (std::string, contents)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::org_def,
    (uint16_t, location)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::struct_def,
    (rc_assembler::symbol, symbol)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::struct_member_def,
    (rc_assembler::symbol, symbol),
    (uint16_t, size)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::end_struct_def,
    (std::string, contents)
)

BOOST_FUSION_ADAPT_STRUCT(
    rc_assembler::assembly_line,
    (std::optional<rc_assembler::line_options>, line_options),
    (std::optional<std::string>, comment)
)
