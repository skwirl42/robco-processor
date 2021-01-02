#pragma once

#include <boost/program_options.hpp>

inline void conflicting_options(boost::program_options::variables_map& variables, const char* option1, const char* option2)
{
    if (variables.count(option1) && !variables[option1].defaulted()
        && variables.count(option2) && !variables[option2].defaulted())
    {
        throw std::logic_error(std::string("Option ") + option1 + " cannot be used with option " + option2);
    }
}

inline void option_dependency(boost::program_options::variables_map& variables, const char* dependent, const char* required)
{
    if (variables.count(dependent) && !variables[dependent].defaulted())
    {
        if (variables.count(required) == 0 || variables[required].defaulted())
        {
            throw std::logic_error(std::string("Option ") + dependent + " requires option " + required + " to be specified");
        }
    }
}

inline void one_of_options_required(boost::program_options::variables_map& variables, std::vector<std::string>& options)
{
    bool any_specified = false;
    for (auto &option : options)
    {
        if (variables.count(option) && !variables[option].defaulted())
        {
            any_specified = true;
            break;
        }
    }

    if (!any_specified)
    {
        auto options_list = boost::join(options, ", ");
        throw std::logic_error("One of the following options must be specified: (" + options_list + ")");
    }
}
