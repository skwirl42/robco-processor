#pragma once

#include <boost/variant.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <string>
#include <vector>

struct ast;
struct binary_expression;

typedef boost::variant<boost::recursive_wrapper<ast>, std::string, int, binary_expression> ast_node;

struct ast
{
    std::vector<ast_node> nodes;
};

struct binary_expression
{
    boost::recursive_wrapper<ast_node> lhs;
    std::string binary_operator;
    boost::recursive_wrapper<ast_node> rhs;
};

BOOST_FUSION_ADAPT_STRUCT(
    binary_expression,
    lhs,
    binary_operator,
    rhs
)
