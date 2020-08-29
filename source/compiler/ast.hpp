#pragma once

#include <boost/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/phoenix.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <tuple>

struct ast_root;
struct binary_expression;

typedef std::vector<int> integer_container;

typedef boost::variant<boost::recursive_wrapper<ast_root>, integer_container, std::string, binary_expression> ast_node;

struct ast_root
{
    std::vector<ast_node> nodes;
};

BOOST_FUSION_ADAPT_STRUCT(
    ast_root,
    nodes
)

struct binary_expression
{
    boost::recursive_wrapper<ast_node> lhs;
    std::string binary_operator;
    boost::recursive_wrapper<ast_node> rhs;
};

// struct binary_expression
// {
//     integer_container lhs;
//     std::string binary_operator;
//     integer_container rhs;
// };

BOOST_FUSION_ADAPT_STRUCT(
    binary_expression,
    lhs,
    binary_operator,
    rhs
)

class ast_visitor : public boost::static_visitor<>
{
public:
    void operator()(const ast_root &root)
    {
        std::cout << "root" << std::endl;
    }

    void operator()(const std::string &string)
    {
        std::cout << string << std::endl;
    }

    void operator()(const integer_container &container)
    {
        std::cout << "integers: ";
        for (auto i : container)
        {
            std::cout << i << ", ";
        }
        std::cout << std::endl;
    }

    void operator()(const binary_expression &expression)
    {
        std::cout   << "expression using '" << expression.binary_operator << "'" << std::endl
                    << "lhs =>" << std::endl
                    ;
        boost::apply_visitor(*this, expression.lhs.get());
        std::cout   << "rhs =>" << std::endl
                    ;
        boost::apply_visitor(*this, expression.rhs.get());
    }
};
