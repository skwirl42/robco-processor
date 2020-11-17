#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <filesystem>

#ifdef _MSC_VER
#include "XGetopt.h"
#else
#include <getopt.h>
#endif

#include "holotape_wrapper.hpp"

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

void usage()
{
    fprintf(stdout, "I'd love to tell you how to use this, but even I don't know yet\n");
}

int main(int argc, char **argv)
{
    char tape_name[TAPE_NAME_LENGTH + 1];
    int c = 0;
    int option_index = 0;
    bool show_usage = false;
    TapeCommand command = TapeCommand::None;

    tape_name[0] = 0;

    while (c >= 0)
    {
        c = getopt(argc, argv, "HC:T:");
        if (c == -1)
        {
            break;
        }

        if (c == 'T')
        {
            // Tape filename
            strncpy(tape_name, optarg, TAPE_NAME_LENGTH);
        }
        else if (c == 'C')
        {
            // Command
            auto command_location = commands.find(optarg);
            if (command_location != commands.end())
            {
                command = (*command_location).second;
            }
            else
            {
                command = TapeCommand::Error;
            }
        }
        else if (c == 'H')
        {
            show_usage = true;
        }
    }

    if (show_usage || strlen(tape_name) == 0)
    {
        usage();
        return 0;
    }

    if (command == TapeCommand::Error || command == TapeCommand::None)
    {
        fprintf(stderr, "Invalid or missing command\n");
        usage();
        return -1;
    }

    std::vector<std::string> paths;
    if (optind < argc)
    {
        // Collect any filenames
        for (int i = optind; i < argc; i++)
        {
            paths.push_back(std::string(argv[i]));
        }
    }

    holotape_wrapper wrapper{ tape_name };
    std::filesystem::path extract_directory;

    switch (command)
    {
    case TapeCommand::Append:
        if (paths.empty())
        {
            fprintf(stderr, "No files specified to append\n");
            usage();
            return -1;
        }

        for (auto filename : paths)
        {
            wrapper.append_file(filename.c_str());
        }
        break;

    case TapeCommand::Erase:
        wrapper.erase();
        break;

    case TapeCommand::Extract:
        if (paths.size() != 1)
        {
            fprintf(stderr, "You must specify a directory to extract the files from the tape into\n");
            usage();
            return -1;
        }
        extract_directory = *paths.begin();
        if (!std::filesystem::is_directory(extract_directory))
        {
            fprintf(stderr, "Please specify a valid directory to extract into\n");
            usage();
            return -1;
        }
        wrapper.extract((*paths.begin()).c_str());
        break;

    case TapeCommand::List:
        wrapper.list();
        break;

    default:
        fprintf(stderr, "Unknown or missing command\n");
        usage();
        return -1;
    }

    return 0;
}