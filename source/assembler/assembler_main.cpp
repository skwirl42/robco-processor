#include "assembler.h"
#include "opcodes.h"

#include <filesystem>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

void usage(char** argv, po::options_description& options)
{
    std::filesystem::path command_path{ argv[0] };
    std::cout << "Usage: " << command_path.filename().string() << " [options]" << std::endl;
    std::cout << options << std::endl;
}

int main(int argc, char **argv)
{
    std::string font_name{};
    po::options_description cli_options("Allowed options");
    cli_options.add_options()
        ("help,?", "output the help message")
        ("include,I", po::value< std::vector < std::string>>(), "directories to include when assembling source")
        ("source,S", po::value<std::string>()->required(), "assembly source file to run")
        ("output-file,O", po::value<std::string>(), "a file into which assembled data is saved")
        ;

    po::variables_map variables;
    po::store(po::command_line_parser(argc, argv).options(cli_options).run(), variables);
    po::notify(variables);

    if (variables.count("help") > 0)
    {
        usage(argv, cli_options);
        return 1;
    }

    const char **includes = nullptr;
    if (variables.count("include") > 0)
    {
        includes = new const char* [variables.count("include") + 1];

        for (int i = 0; i < variables.count("include"); i++)
        {
            includes[i] = variables["include"].as<std::vector<std::string>>()[i].c_str();
        }

        includes[variables.count("include")] = 0;
    }

    if (variables.count("source") > 0)
    {
        assembler_data_t *assembled_data;
        const char* output_file = nullptr;
        
        if (variables.count("output-file") > 0)
        {
            variables["output-file"].as<std::string>().c_str();
        }

        assemble(variables["source"].as<std::string>().c_str(), includes, output_file, &assembled_data);
        if (get_error_buffer_size(assembler_data) > 0)
        {
            printf("%s\n", error_buffer);
            return -1;
        }
        else
        {
            printf("Program assembled successfully\n");
        }
    }
    else
    {
        printf("A source file must be specified\n");
        usage(argv, cli_options);
        return -1;
    }
    
    return 0;
}