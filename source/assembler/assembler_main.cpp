#include "assembler.h"
#include "opcodes.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <iterator>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "assembler_parser.hpp"

namespace po = boost::program_options;


namespace
{
    class line_visitor : public boost::static_visitor<>
    {
    public:
        void operator()(const rc_assembler::instruction_line& instruction)
        {
            std::cout << "instruction: " << instruction.opcode->name << std::endl;
        }

        void operator()(const rc_assembler::reservation& reservation)
        {
            std::cout << "reservation: " << reservation.symbol << " size: " << reservation.size << std::endl;
        }

        void operator()(const rc_assembler::byte_def& byte)
        {

        }

        void operator()(const rc_assembler::word_def& word)
        {

        }

        void operator()(const rc_assembler::data_def& def)
        {

        }

        void operator()(const rc_assembler::org_def& def)
        {

        }

        void operator()(const rc_assembler::label_def& def)
        {

        }

        void operator()(const rc_assembler::include& def)
        {
            std::string string(def.included_file.begin(), def.included_file.end());
            std::cout << "including: " << string << std::endl;
        }
    };

}

std::istream& operator>>(std::istream& in, OutputFileType& fileType)
{
    std::string token;
    in >> token;

    boost::to_lower(token);

    if (token == "binary" || token == "b")
    {
        fileType = OutputFileType::Binary;
    }
    else if (token == "summary" || token == "s")
    {
        fileType = OutputFileType::Summary;
    }
    else
    {
        fileType = OutputFileType::Error;
    }
    
    return in;
}

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
    OutputFileType outFileType = Binary;
    cli_options.add_options()
        ("help,?", "output the help message")
        ("include,I", po::value< std::vector < std::string>>(), "directories to include when assembling source")
        ("source,S", po::value<std::string>()->required(), "assembly source file to run")
        ("output-file,O", po::value<std::string>(), "a file into which assembled data is saved")
        ("type,T", po::value<OutputFileType>(&outFileType)->default_value(Binary), "what type of file to output (binary or summary)")
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
        rc_assembler::assembler_grammar<std::string::iterator> parser;
        auto& source_file = variables["source"].as<std::string>();
        std::ifstream file_stream(source_file, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
        auto file_size = file_stream.tellg();
        file_stream.seekg(0);
        std::string file_contents(file_size, '\0');
        file_stream.read(&file_contents[0], file_size);

        line_visitor visitor{};
        rc_assembler::assembly_file assembled;
        if (boost::spirit::qi::phrase_parse(file_contents.begin(), file_contents.end(), parser, boost::spirit::ascii::blank, assembled))
        {
            for (auto &line : assembled)
            {
                boost::apply_visitor(visitor, line);
            }
        }

        assembler_data_t *assembled_data;
        const char* output_file = nullptr;
        
        if (variables.count("output-file") > 0)
        {
            output_file = variables["output-file"].as<std::string>().c_str();
        }

        assemble(variables["source"].as<std::string>().c_str(), includes, output_file, outFileType, &assembled_data);
        if (get_error_buffer_size(assembler_data) > 0)
        {
            std::cerr << error_buffer << std::endl;
            return -1;
        }
        else
        {
            std::cout << "Program assembled successfully into " << get_output_filename(assembled_data) << std::endl;
        }
    }
    else
    {
        std::cerr << "A source file must be specified" << std::endl;
        usage(argv, cli_options);
        return -1;
    }
    
    return 0;
}