#pragma once

#include <boost/spirit/include/qi.hpp>
#include "ast.hpp"

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

template <typename Iterator>
struct translation_unit : qi::grammar<Iterator, ast(), ascii::space_type>
{
    translation_unit() : translation_unit::base_type(ast)
    {
        ast %= +ast_node;
        ast_node %= identifier
                    | integer
                    | binary_expression
                    | ast_node
                    ;
        identifier %= qi::lexeme[qi::char_("a-zA-Z_") >> *qi::char_("_a-zA-Z0-9")];
        integer %= qi::int_
                 | qi::lexeme['0' >> qi::char_("xX") >> +qi::hex]
                 | qi::lexeme['0' >> +qi::oct]
                 ;
        binary_expression %=    ast_node >> arithmetic_operator >> ast_node
                            |   ast_node >> logical_operator >> ast_node
                            |   ast_node >> comparison_operator >> ast_node
                            |   ast_node >> assignment_operator >> ast_node
                            ;
        arithmetic_operator %=  qi::string("-")
                            |   qi::string("+")
                            |   qi::string("/")
                            |   qi::string("*")
                            |   qi::string("%")
                            |   qi::string("^")
                            |   qi::string("&")
                            |   qi::string("|")
                            ;
        logical_operator %=     qi::string("||")
                            |   qi::string("&&")
                            ;
        comparison_operator %=      qi::string("<")
                                |   qi::string(">")
                                |   qi::string("==")
                                |   qi::string("!=")
                                |   qi::string(">=")
                                |   qi::string("<=")
                                ;
        assignment_operator %=      qi::string("=")
                                |   qi::string("+=")
                                |   qi::string("-=")
                                |   qi::string("*=")
                                |   qi::string("/=")
                                |   qi::string("%=")
                                |   qi::string("^=")
                                |   qi::string("&=")
                                |   qi::string("|=")
                                |   qi::string("&&=")
                                |   qi::string("||=")
                                ;
    }

    qi::rule<Iterator, ast(), ascii::space_type> ast;
    qi::rule<Iterator, ast_node(), ascii::space_type> ast_node;
    qi::rule<Iterator, std::string(), ascii::space_type> identifier;
    qi::rule<Iterator, int, ascii::space_type> integer;
    qi::rule<Iterator, binary_expression(), ascii::space_type> binary_expression;
    qi::rule<Iterator, std::string(), ascii::space_type> arithmetic_operator;
    qi::rule<Iterator, std::string(), ascii::space_type> logical_operator;
    qi::rule<Iterator, std::string(), ascii::space_type> comparison_operator;
    qi::rule<Iterator, std::string(), ascii::space_type> assignment_operator;
};
