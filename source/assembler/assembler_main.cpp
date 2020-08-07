#include "assembler.h"
#include "opcodes.h"
#include <getopt.h>

int main(int argc, char **argv)
{
    char includes_buffer[LINE_BUFFER_SIZE+1];
    includes_buffer[0] = 0;

    static struct option long_options[] =
    {
        { "include-dirs", required_argument, 0, 'I' },
        { 0, 0, 0, 0 }
    };
    int c = 0;
    int option_index = 0;
    while (c >= 0)
    {
        c = getopt_long(argc, argv, "I:", long_options, &option_index);
        if (c == -1)
        {
            break;
        }

        if (c == 'I')
        {
            strncpy(includes_buffer, optarg, LINE_BUFFER_SIZE);
        }
    }

    const char **includes;
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
        if (assembled_data->errors.size() > 0)
        {
            printf("%s\n", assembled_data->error_buffer);
            return -1;
        }
    }

    return 0;
}