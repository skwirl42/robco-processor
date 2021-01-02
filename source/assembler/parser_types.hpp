#pragma once

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <optional>

#include "opcodes.h"
#include "assembler_internal.h"

namespace rc_assembler
{
    namespace fusion = boost::fusion;
    namespace phoenix = boost::phoenix;
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

    struct include
    {
        byte_array included_file;
    };

    struct label_def
    {
        symbol label_name;
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

    struct org_def
    {
        uint16_t location;
    };

    typedef boost::variant<instruction_line,reservation,byte_def,word_def,data_def,org_def,label_def,include> assembly_line;

    typedef std::vector<assembly_line> assembly_file;

 }

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
    rc_assembler::include,
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
    rc_assembler::org_def,
    (uint16_t, location)
)