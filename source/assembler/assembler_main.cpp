#include "assembler.h"
#include "opcodes.h"
#ifdef _MSC_VER
#include "XGetopt.h"
#else
#include <getopt.h>
#endif

void usage()
{
    printf("Usage:\n");
    printf("assembler [--include-dirs <dir1>[;<dir2>[;<dir3>] ..] <input file> [<output file>]\n");
    printf("\t--include-dirs: a semi-colon delimited list of directories where include files can be found\n");
    printf("\t<output file>: a file to write the assembled program into. Defaults to \"assembled.txt\"\n");
}

int main(int argc, char **argv)
{
    char includes_buffer[LINE_BUFFER_SIZE+1];
    includes_buffer[0] = 0;

#ifndef _MSC_VER
    static struct option long_options[] =
    {
        { "help", no_argument, 0, 'H' },
        { "include-dirs", required_argument, 0, 'I' },
        { 0, 0, 0, 0 }
    };
#endif

    int c = 0;
    int option_index = 0;
    bool show_usage = false;
    while (c >= 0)
    {
#ifdef _MSC_VER
        c = getopt(argc, argv, "HI:");
#else
        c = getopt_long(argc, argv, "HI:", long_options, &option_index);
#endif
        if (c == -1)
        {
            break;
        }

        if (c == 'I')
        {
            strncpy(includes_buffer, optarg, LINE_BUFFER_SIZE);
        }
        else if (c == 'H')
        {
            show_usage = true;
        }
    }

    if (show_usage)
    {
        usage();
    }

    const char **includes = nullptr;
    if (strlen(includes_buffer) > 0)
    {
        int length = strlen(includes_buffer);
        int dir_count = 1;
        for (int i = 0; i < length; i++)
        {
            if (includes_buffer[i] == ';')
            {
                includes_buffer[i] = 0;
                dir_count++;
            }
        }
        includes = new const char*[dir_count + 1];
        includes[dir_count] = 0;
        int current_index = 0;
        for (int i = 0; i < dir_count; i++)
        {
            includes[i] = &includes_buffer[current_index];
            while (current_index < length && includes_buffer[current_index] != 0)
            {
                current_index++;
            }
        }
    }

    if (optind < argc)
    {
        assembler_data_t *assembled_data;
        const char *output_file = nullptr;
        if (optind + 1 < argc)
        {
            output_file = argv[optind+1];
        }

        assemble(argv[optind], includes, output_file, &assembled_data);
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
    else if (!show_usage)
    {
        usage();
    }
    
    return 0;
}