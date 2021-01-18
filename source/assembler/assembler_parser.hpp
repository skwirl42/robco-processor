#pragma once

#include "parser_types.hpp"

#include <boost/phoenix/object/construct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi_uint.hpp>

// Summary:
//
// \s refers to non-end-of-line characters, as the assembler parses a single line at a time
//
// Hex byte literal
// 0(x|X)<hex digit>{1,2}
//
// Hex integer literal
// 0(x|X)<hex digit>{1,4}
//
// Byte sequence
// <hex byte literal>[\s+<hex byte literal>]*
//
// Instruction line
// \s*<instruction>[\s+(<symbol>|<single-value literal>|<register index>)]
//
// Byte/word definition
// .<def directive>\s+<symbol>\s+<integer literal>
//
// Data definition
// .data\s+[<symbol>\s+](<quoted string>|<byte sequence>)
// (optional) <byte sequence>
// .enddata
//
// Structure definition
// .struct\s+<symbol>
// <symbol>\s+<size:unsigned integer literal>
// .endstruct
//
// Reservation
// .reserve\s+<symbol>\s+<integer literal>
//
// Include directive
// .include\s+<quoted filename>
//
// Org directive
// .org\s+<integer literal>
//
// Register index - pre-increment/decrement
// [\s*(+|-){0,2}\s*<register>\s*]
// - post-increment/decrement
// [\s*<register>\s*(+|-){1,2}\s*]
//
// Comment
// ;<character>*

// Macros
// Minimum set of macros :
//
// -`set(w) <index register> \[<0:symbol or immediate>\] <1:symbol or immediate>`
// - if value of <0> is 0:
//   - push <1> to stack
//   - pull to location of index register
// - if value of 0 is not 0 :
//   - push the index register to the stack
//   - dup
//   - push <0> to stack
//   - add
//   - pull index register
//   - push <1>
//   - pull to location of index register
//   - pull the index register
// - `get(w) <index register> \[<symbol or immediate index>\]`
// - if value is 0:
//   - pull location at index register
// - if value is not 0 :
//   - pushes the index register to the stack
//   - dup
//   - push value
//   - add
//   - pull index register
//   - push value at index register to stack
//   - swap the value and original index register
//   - pull to the index register

namespace rc_assembler
{
    namespace phoenix = boost::phoenix;
    using qi::_val;

    template<typename Iterator>
    struct assembler_grammar : qi::grammar<Iterator, assembly_line(), ascii::space_type>
    {
        assembler_grammar() : assembler_grammar::base_type(line_rule)
        {
            hex_byte_lit %= '0' >> qi::no_case['x'] >> hex_byte_parser;
            hex_word_lit %= '0' >> qi::no_case['x'] >> hex_word_parser;

            escaped_char
                .add("\\n", (uint8_t)'\n')
                ("\\t", (uint8_t)'\t')
                ("\\\\", (uint8_t)'\\')
                ("\\\"", (uint8_t)'\"')
                ("\\\'", (uint8_t)'\'')
                ;

            quoted_byte_string %= qi::lit('"') >> qi::lexeme[*(escaped_char | (qi::print - '"' - '\\') | ("\\x" >> hex_escape_parser))] >> qi::lit('"')[phoenix::push_back(_val, 0)];

            symbol_rule %= qi::lexeme[qi::char_("a-zA-Z") >> *qi::char_(".()_a-zA-Z0-9")];

            byte_sequence %= hex_byte_lit % ascii::space;

            increment_rule =
                qi::string("++")[_val = 2]
                | qi::char_('+')[_val = 1]
                | qi::string("--")[_val = -2]
                | qi::char_('-')[_val = -1];

            indexed_register_argument_rule =
                ('[' >> ascii::no_case[register_parser] >> ']')[_val = phoenix::construct<register_index_t>(qi::_1, 0, 0)]
                | ('[' >> increment_rule >> ascii::no_case[register_parser] >> ']')[_val = phoenix::construct<register_index_t>(qi::_2, 1, qi::_1)]
                | ('[' >> ascii::no_case[register_parser] >> increment_rule >> ']')[_val = phoenix::construct<register_index_t>(qi::_1, 0, qi::_2)];

            instruction_argument_rule %= symbol_rule | hex_word_lit | hex_byte_lit | qi::int_ | indexed_register_argument_rule;

            comment %= ';' >> *qi::char_;
            byte_def_rule %= ".defbyte" >> symbol_rule >> (hex_byte_lit | qi::uint_);
            word_def_rule %= ".defword" >> symbol_rule >> (hex_word_lit | qi::uint_);
            data_def_rule %= ".data" >> -symbol_rule >> (byte_sequence | quoted_byte_string);
            bytes_rule %= byte_sequence;
            end_data_rule %= ".enddata" >> *qi::char_;
            reservation_rule %= ".reserve" >> symbol_rule >> (hex_word_lit | qi::uint_);
            include_rule %= ".include" >> quoted_byte_string;
            label_def_rule %= symbol_rule >> ':';
            org_def_rule %= ".org" >> (hex_word_lit | qi::uint_);

            struct_def_rule %= ".struct" >> symbol_rule;
            struct_member_rule %= symbol_rule >> qi::uint_;
            end_struct_rule %= ".endstruct" >> *qi::char_;

            opcode_rule %= qi::lexeme[ascii::no_case[opcode_parser >> !(ascii::alnum | '_' | '.')]];

            instruction_line_rule %= opcode_rule >> -instruction_argument_rule;

            line_options_rule %=
                (
                    instruction_line_rule
                    | reservation_rule
                    | byte_def_rule
                    | word_def_rule
                    | data_def_rule
                    | bytes_rule
                    | end_data_rule
                    | org_def_rule
                    | label_def_rule
                    | include_rule
                    | struct_def_rule
                    | struct_member_rule
                    | end_struct_rule
                );

            line_rule %= (line_options_rule || comment);

            opcode_parser.name("opcodes");
            int opcode_count = opcode_entry_count();
            for (int i = 0; i < opcode_count; i++)
            {
                opcode_entry_t* opcode = get_opcode_entry_by_index(i);
                if (opcode == nullptr)
                {
                    std::cout << "Tried to add an option at index " << i << " but it was null" << std::endl;
                }
                else
                {
                    opcode_parser.add(opcode->name, opcode);
                }
            }

            register_parser.name("registers");
            int register_entry_count = register_count();
            for (int i = 0; i < register_entry_count; i++)
            {
                register_argument_t* register_arg = get_register_by_index(i);
                register_parser.add(register_arg->name, register_arg);
            }
        }

        qi::symbols<char, opcode_entry_t*> opcode_parser;
        qi::symbols<char, register_argument_t*> register_parser;
        qi::symbols<char, uint8_t> escaped_char;

        qi::uint_parser<uint8_t, 16, 2, 2> hex_escape_parser;
        qi::uint_parser<uint8_t, 16, 1, 2> hex_byte_parser;
        qi::uint_parser<uint16_t, 16, 1, 4> hex_word_parser;

        qi::rule<Iterator, int, ascii::space_type> increment_rule;
        qi::rule<Iterator, register_index_t(), ascii::space_type> indexed_register_argument_rule;
        qi::rule<Iterator, instruction_argument(), ascii::space_type> instruction_argument_rule;

        qi::rule<Iterator, uint8_t(), ascii::space_type> hex_byte_lit;
        qi::rule<Iterator, uint16_t(), ascii::space_type> hex_word_lit;
        qi::rule<Iterator, byte_array(), ascii::space_type> byte_sequence;
        qi::rule<Iterator, byte_array(), ascii::space_type> quoted_byte_string;

        qi::rule<Iterator, symbol(), ascii::space_type> symbol_rule;

        qi::rule<Iterator, opcode_entry_t*, ascii::space_type> opcode_rule;
        qi::rule<Iterator, byte_def(), ascii::space_type> byte_def_rule;
        qi::rule<Iterator, word_def(), ascii::space_type> word_def_rule;
        qi::rule<Iterator, data_def(), ascii::space_type> data_def_rule;
        qi::rule<Iterator, byte_array(), ascii::space_type> bytes_rule;
        qi::rule<Iterator, end_data_def(), ascii::space_type> end_data_rule;
        qi::rule<Iterator, reservation(), ascii::space_type> reservation_rule;
        qi::rule<Iterator, include_def(), ascii::space_type> include_rule;
        qi::rule<Iterator, label_def(), ascii::space_type> label_def_rule;
        qi::rule<Iterator, org_def(), ascii::space_type> org_def_rule;
        qi::rule<Iterator, struct_def(), ascii::space_type> struct_def_rule;
        qi::rule<Iterator, struct_member_def(), ascii::space_type> struct_member_rule;
        qi::rule<Iterator, end_struct_def(), ascii::space_type> end_struct_rule;
        qi::rule<Iterator, instruction_line(), ascii::space_type> instruction_line_rule;
        qi::rule<Iterator, line_options(), ascii::space_type> line_options_rule;

        qi::rule<Iterator, std::string(), ascii::space_type> comment;

        qi::rule<Iterator, assembly_line(), ascii::space_type> line_rule;
    };
}
