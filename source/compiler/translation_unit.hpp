#pragma once

#include <boost/spirit/include/qi.hpp>
#include <tuple>

#include "ast.hpp"

namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;
namespace ascii = boost::spirit::ascii;

template <typename Iterator>
struct translation_unit : qi::grammar<Iterator, ast_root(), ascii::space_type>
{
    translation_unit() : translation_unit::base_type(ast)
    {
        ast = +(ast_node[phoenix::push_back(phoenix::bind(&ast_root::nodes, qi::_val), qi::_1)]);
        ast_node %=     binary_expression
                    |   integer
                    |   identifier
                    //| ast
                    ;
        identifier %= qi::lexeme[qi::char_("a-zA-Z_") >> *qi::char_("_a-zA-Z0-9")];
        integer %= qi::lexeme['0' >> qi::char_("xX") >> +qi::hex]
                 | qi::lexeme['0' >> +qi::oct]
                 | qi::int_
                 ;

        binary_expression %=        
                                    identifier >> logical_operator >> ast_node
                                |   integer >> logical_operator >> ast_node
                                
                                |   identifier >> comparison_operator >> ast_node
                                |   integer >> comparison_operator >> identifier

                                |   identifier >> assignment_operator >> ast_node

                                |   identifier >> arithmetic_operator >> ast_node
                                |   integer >> arithmetic_operator >> ast_node
                                ;

        arithmetic_operator %=  
                                qi::string("/")
                            |   qi::string("*")
                            |   qi::string("%")
                            |   qi::string("-")
                            |   qi::string("+")
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
        ast.name("noderoot");
        ast_node.name("node");
        identifier.name("identifier");
        binary_expression.name("binary_expression");
        arithmetic_operator.name("arithmetic_operator");
    }

    qi::rule<Iterator, ast_root(), ascii::space_type> ast;
    qi::rule<Iterator, ast_node(), ascii::space_type> ast_node;
    qi::rule<Iterator, std::string(), ascii::space_type> identifier;
    qi::rule<Iterator, integer_container(), ascii::space_type> integer;
    qi::rule<Iterator, binary_expression(), ascii::space_type> binary_expression;
    qi::rule<Iterator, std::string(), ascii::space_type> arithmetic_operator;
    qi::rule<Iterator, std::string(), ascii::space_type> logical_operator;
    qi::rule<Iterator, std::string(), ascii::space_type> comparison_operator;
    qi::rule<Iterator, std::string(), ascii::space_type> assignment_operator;
};
