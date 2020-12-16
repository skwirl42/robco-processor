#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <filesystem>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include "holotape_wrapper.hpp"
#include "exceptions.hpp"

enum class TapeCommand
{
    None,
    Error,
    Append,
    Erase,
    Extract,
    List,
};

namespace
{
    std::map<std::string, TapeCommand> commands =
    {
        { "append",     TapeCommand::Append },
        { "erase",      TapeCommand::Erase },
        { "extract",    TapeCommand::Extract },
        { "list",       TapeCommand::List },
    };
}

void usage(char** argv, po::options_description& options)
{
    std::filesystem::path command_path{ argv[0] };
    std::cout << "Usage: " << command_path.filename().string() << " [options]" << std::endl;
    std::cout << options << std::endl;
}

int main(int argc, char **argv)
{
    std::filesystem::path extract_directory;
    std::vector<std::string> paths;
    po::options_description cli_options("Allowed options");
    cli_options.add_options()
        ("help,?", "output the help message")
        ("command,K", po::value<std::string>()->required(), "command to execute")
        ("tape,T", po::value<std::string>()->required(), "tape to manage")
        ("file,F", po::value<std::vector<std::string>>(&paths), "files to append")
        ("outdir,O", po::value<std::filesystem::path>(&extract_directory), "directory to extract tape contents to")
        ;

    try
    {
        po::variables_map variables;
        po::store(po::command_line_parser(argc, argv).options(cli_options).run(), variables);
        po::notify(variables);

        if (variables.count("help") > 0)
        {
            usage(argv, cli_options);
            return 0;
        }

        TapeCommand command = TapeCommand::None;

        if (variables.count("command") == 0)
        {
            fprintf(stderr, "A command must be specified\n");
            usage(argv, cli_options);
            return -1;
        }

        if (variables.count("tape") == 0)
        {
            fprintf(stderr, "A tape file must be specified\n");
            usage(argv, cli_options);
            return -1;
        }

        auto command_location = commands.find(variables["command"].as<std::string>());
        if (command_location != commands.end())
        {
            command = (*command_location).second;
        }
        else
        {
            command = TapeCommand::Error;
        }

        if (command == TapeCommand::Error || command == TapeCommand::None)
        {
            fprintf(stderr, "Invalid or missing command\n");
            usage(argv, cli_options);
            return -1;
        }

        holotape_wrapper wrapper{ variables["tape"].as<std::string>().c_str() };

        switch (command)
        {
        case TapeCommand::Append:
            if (paths.empty())
            {
                fprintf(stderr, "No files specified to append\n");
                usage(argv, cli_options);
                return -1;
            }

            for (auto& filename : paths)
            {
                wrapper.append_file(filename.c_str());
            }
            break;

        case TapeCommand::Erase:
            wrapper.erase();
            break;

        case TapeCommand::Extract:
            if (variables.count("outdir") != 1)
            {
                fprintf(stderr, "You must specify a directory to extract the files from the tape into\n");
                usage(argv, cli_options);
                return -1;
            }
            if (!std::filesystem::is_directory(extract_directory))
            {
                fprintf(stderr, "Please specify a valid directory to extract into\n");
                usage(argv, cli_options);
                return -1;
            }
            wrapper.extract(extract_directory.string().c_str());
            break;

        case TapeCommand::List:
            wrapper.list();
            break;

        default:
            fprintf(stderr, "Unknown or missing command\n");
            usage(argv, cli_options);
            return -1;
        }
    }
    catch (boost::wrapexcept<po::required_option>& exception)
    {
        std::cerr << "Missing required option " << exception.get_option_name() << std::endl;
        usage(argv, cli_options);
        return -1;
    }
    catch (basic_error& error_exception)
    {
        std::string const* message = boost::get_error_info<error_message>(error_exception);
        if (message != nullptr)
        {
            std::cerr << "Error message: " << message << std::endl;
        }
        else
        {
            std::cerr << "Exception missing error message" << std::endl;
        }
        return -1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
        return -1;
    }

    return 0;
}