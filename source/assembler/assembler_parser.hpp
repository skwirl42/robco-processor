#pragma once

#include "parser_types.hpp"

#include <boost/phoenix/object/construct.hpp>

// Summary:
//
// \s refers to non-end-of-line characters
//
// End-of-line <eol>
// (<cr>|<lf>|<eof>)+
//
// Hex byte literal
// 0(x|X)<hex digit>{1,2}
//
// Hex integer literal
// 0(x|X)<hex digit>{1,4}
//
// Byte sequence - note: the whitespace can include <eol> characters, as long as each line begins with other white space
// <hex byte literal>[\s+<hex byte literal>]*<eol>
//
// Instruction line
// \s+<instruction>[\s+(<symbol>|<single-value literal>|<register index>)]<eol>
//
// Byte/word definition
// .<def directive>\s+<symbol>\s+<integer literal><eol>
//
// Data definition
// .data\s+[<symbol>\s+](<quoted string>|<byte sequence>)
//
// Reservation
// .reserve\s+<symbol>\s+<integer literal><eol>
//
// Include directive
// .include\s+<quoted filename><eol>
//
// Org directive
// .org\s+<integer literal><eol>
//
// Register index - pre-increment/decrement
// [\s*(+|-){0,2}\s*<register>\s*]
// - post-increment/decrement
// [\s*<register>\s*(+|-){1,2}\s*]
//
// Comment
// ;[<characters except EOL>]*<eol>

namespace rc_assembler
{
    using qi::_val;

    template<typename Iterator>
    struct assembler_grammar : qi::grammar<Iterator, assembly_file(), ascii::blank_type>
    {
        assembler_grammar() : assembler_grammar::base_type(file_rule)
        {
            hex_byte_lit %= qi::lexeme['0' >> qi::char_("xX") >> hex_byte_parser];
            hex_word_lit %= qi::lexeme['0' >> qi::char_("xX") >> hex_word_parser];

            escaped_char
                .add("\\n", (uint8_t)'\n')
                    ("\\t", (uint8_t)'\t')
                    ("\\\\", (uint8_t)'\\')
                    ("\\\"", (uint8_t)'\"')
                    ("\\\'", (uint8_t)'\'')
                ;

            quoted_byte_string %= qi::lit('"') >> *(escaped_char | (qi::print - '"' - '\\') | "\\x" >> hex_escape_parser) >> qi::lit('"') [phoenix::push_back(_val, 0)];
            
            symbol_rule %= qi::lexeme[qi::char_("a-zA-Z") >> *qi::char_("a-zA-Z0-9_")];
            
            byte_sequence %= hex_byte_lit %  ascii::space ;     

            increment_rule =
                  qi::string("++")  [_val = 2]
                | qi::char_('+')    [_val = 1]
                | qi::string("--")  [_val = -2]
                | qi::char_('-')    [_val = -1];

            indexed_register_argument_rule = 
                  ('[' >> ascii::no_case[register_parser] >> ']')                     [_val = phoenix::construct<register_index_t>(qi::_1, 0, 0)]
                | ('[' >> increment_rule >> ascii::no_case[register_parser] >> ']')   [_val = phoenix::construct<register_index_t>(qi::_2, 1, qi::_1)]
                | ('[' >> ascii::no_case[register_parser] >> increment_rule >> ']')   [_val = phoenix::construct<register_index_t>(qi::_1, 0, qi::_2)];

            instruction_argument_rule %= symbol_rule | hex_word_lit | hex_byte_lit | qi::int_ | indexed_register_argument_rule;

            comment %= ';' >> *(!qi::eol);
            byte_def_rule %= ".defbyte" >> symbol_rule >> hex_byte_lit;
            word_def_rule %= ".defword" >> symbol_rule >> hex_word_lit;
            data_def_rule %= ".data" >> -symbol_rule >> (byte_sequence | quoted_byte_string);
            reservation_rule %= ".reserve" >> symbol_rule >> (hex_word_lit | qi::uint_);
            include_rule %= ".include" >> quoted_byte_string;
            label_def_rule %= symbol_rule >> ':';
            org_def_rule %= ".org" >> (hex_word_lit | qi::uint_);

            instruction_line_rule %= qi::no_case[opcode_parser] >> -instruction_argument_rule;

            line_options_rule %= (instruction_line_rule | reservation_rule | byte_def_rule | word_def_rule | data_def_rule | org_def_rule | label_def_rule | include_rule);
            // instruction_line,reservation,byte_def,word_def,data_def,org_def,label_def,include
            line_rule %= (line_options_rule || comment) >> qi::eol;

            file_rule %= +(+qi::eol | line_rule) >> qi::eoi;

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

        qi::rule<Iterator, int, ascii::blank_type> increment_rule;
        qi::rule<Iterator, register_index_t(), ascii::blank_type> indexed_register_argument_rule;
        qi::rule<Iterator, instruction_argument(), ascii::blank_type> instruction_argument_rule;

        qi::rule<Iterator, uint8_t(), ascii::blank_type> hex_byte_lit;
        qi::rule<Iterator, uint16_t(), ascii::blank_type> hex_word_lit;
        qi::rule<Iterator, byte_array(), ascii::blank_type> byte_sequence;
        qi::rule<Iterator, byte_array(), ascii::blank_type> quoted_byte_string;

        qi::rule<Iterator, symbol(), ascii::blank_type> symbol_rule;

        qi::rule<Iterator, byte_def(), ascii::blank_type> byte_def_rule;
        qi::rule<Iterator, word_def(), ascii::blank_type> word_def_rule;
        qi::rule<Iterator, data_def(), ascii::blank_type> data_def_rule;
        qi::rule<Iterator, reservation(), ascii::blank_type> reservation_rule;
        qi::rule<Iterator, include(), ascii::blank_type> include_rule;
        qi::rule<Iterator, label_def(), ascii::blank_type> label_def_rule;
        qi::rule<Iterator, org_def(), ascii::blank_type> org_def_rule;
        qi::rule<Iterator, instruction_line(), ascii::blank_type> instruction_line_rule;
        qi::rule<Iterator, line_options(), ascii::blank_type> line_options_rule;

        qi::rule<Iterator, std::string(), ascii::blank_type> comment;

        qi::rule<Iterator, assembly_line(), ascii::blank_type> line_rule;

        qi::rule<Iterator, assembly_file(), ascii::blank_type> file_rule;
    };
}
