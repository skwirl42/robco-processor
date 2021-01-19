#include "holotape_wrapper.hpp"

#include <stdio.h>
#include <math.h>
#include <filesystem>

holotape_wrapper::holotape_wrapper(const char *filename) : filename(filename), initialized(false)
{
    deck = holotape_deck_init();
    if (deck)
    {
        auto result = holotape_insert(deck, filename);
        if (result != HOLO_NO_ERROR)
        {
            fprintf(stderr, "Failed to insert the tape!\n");
            return;
        }

        initialized = true;
    }
    else
    {
        fprintf(stderr, "Failed to initialize the deck!\n");
    }
}

holotape_wrapper::~holotape_wrapper()
{
    holotape_deck_dispose(deck);
}

void holotape_wrapper::append_file(const char *file_to_append)
{
    if (!initialized)
    {
        fprintf(stderr, "Cannot append file %s as the deck is not properly initialized\n", file_to_append);
        return;
    }

    std::filesystem::path file_path(file_to_append);

    if (!std::filesystem::exists(file_path) || !std::filesystem::is_regular_file(file_path))
    {
        fprintf(stderr, "Cannot append \"%s\" because it either doesn't exist, or isn't a regular file\n", file_to_append);
        return;
    }

    auto filename = file_path.filename();
    auto filename_string = filename.string();
    char filename_buffer[HOLOTAPE_FILE_NAME_MAX + 1];
    strncpy(filename_buffer, (const char *)filename_string.c_str(), HOLOTAPE_FILE_NAME_MAX);
    for (int i = filename_string.size(); i <= HOLOTAPE_FILE_NAME_MAX; i++)
    {
        filename_buffer[i] = 0;
    }

    if (filename_string.size() > HOLOTAPE_FILE_NAME_MAX)
    {
        fprintf(stderr, "Appended file name has been truncated to %s\n", filename_buffer);
    }

    FILE* file = 0;
#if defined(_MSC_VER)
    file = fopen(file_to_append, "rb");
#else
    file = fopen(file_to_append, "r");
#endif
    if (file == 0)
    {
        fprintf(stderr, "Failed to open \"%s\" for appending\n", file_to_append);
        return;
    }

    // Find the first free block on the tape
    holotape_rewind(deck);
    bool found_block = false;
    holotape_status_t read_status = HOLO_NO_ERROR;
    do {
        read_status = holotape_read(deck);
        if (read_status == HOLO_NO_ERROR && deck->block_buffer.block_structure.filename[0] == 0)
        {
            found_block = true;
            break;
        }
        else if (read_status != HOLO_NO_ERROR && read_status != HOLO_END_OF_TAPE)
        {
            break;
        }
    } while(read_status != HOLO_END_OF_TAPE);

    if (found_block)
    {
        holotape_rewind_block(deck);
    }
    else if (read_status == HOLO_END_OF_TAPE)
    {
        fprintf(stderr, "No space left on tape\n");
        fclose(file);
        return;
    }
    else
    {
        fprintf(stderr, "Received an error when trying to find an empty block on the tape\n");
        fclose(file);
        return;
    }
    
    memcpy(deck->block_buffer.block_structure.filename, filename_buffer, HOLOTAPE_FILE_NAME_MAX);

    auto current_size = std::filesystem::file_size(file_path);
    if (current_size > HOLOTAPE_STRUCTURE_BYTE_COUNT)
    {
        int block_count = current_size / HOLOTAPE_STRUCTURE_BYTE_COUNT;
        if ((block_count * HOLOTAPE_STRUCTURE_BYTE_COUNT) < current_size)
        {
            block_count++;
        }

        int blocks_remaining = block_count - 1;
        while (current_size > 0)
        {
            size_t read_size = current_size > HOLOTAPE_STRUCTURE_BYTE_COUNT ? HOLOTAPE_STRUCTURE_BYTE_COUNT : current_size;
            
            uint8_t current_block = 0;
            if (holotape_get_current_block(deck, &current_block) != HOLO_NO_ERROR)
            {
                
            }
            
            deck->block_buffer.block_structure.block_bytes.word = read_size + HOLOTAPE_HEADER_SIZE;
            deck->block_buffer.block_structure.remaining_blocks_in_file = blocks_remaining--;
            deck->block_buffer.block_structure.next_block = (blocks_remaining > 0) ? current_block + 1 : 0;
            auto size_read = fread(deck->block_buffer.block_structure.bytes, 1, read_size, file);

            if (size_read == read_size)
            {
                auto status = holotape_write(deck);
                if (status != HOLO_NO_ERROR)
                {
                    fprintf(stderr, "Failed to write the appended block\n");
                }
            }
            else
            {
                fprintf(stderr, "Could not read all bytes from a file (%s) to be appended\n", filename_string.c_str());
            }

            memset(deck->block_buffer.block_structure.bytes, 0, HOLOTAPE_STRUCTURE_BYTE_COUNT);
            current_size -= size_read;
        }
    }
    else
    {
        deck->block_buffer.block_structure.block_bytes.word = current_size + HOLOTAPE_HEADER_SIZE;
        deck->block_buffer.block_structure.remaining_blocks_in_file = 0;
        deck->block_buffer.block_structure.next_block = 0;
        auto size_read = fread(deck->block_buffer.block_structure.bytes, 1, current_size, file);

        if (size_read == current_size)
        {
            auto status = holotape_write(deck);
            if (status != HOLO_NO_ERROR)
            {
                fprintf(stderr, "Failed to write the appended block\n");
            }
        }
        else
        {
            if (feof(file))
            {
                fprintf(stderr, "Unexpected EOF\n");
            }
            fprintf(stderr, "Could not read all bytes from a file (%s) to be appended\n", filename_string.c_str());
        }
    }

    fclose(file);
}

void holotape_wrapper::erase()
{
    if (!initialized)
    {
        fprintf(stderr, "Cannot erase the contents of %s as initialization failed\n", filename);
        return;
    }

    // Just blast HOLOTAPE_MAX_BLOCKS blocks of blankness
    fprintf(stdout, "Erasing holotape %s\n", filename);
    holotape_rewind(deck);
    memset(deck->block_buffer.buffer, 0, sizeof(deck->block_buffer.buffer));
    for (int i = 0; i < HOLOTAPE_MAX_BLOCKS; i++)
    {
        auto write_result = holotape_write(deck);
        if (write_result != HOLO_NO_ERROR)
        {
            fprintf(stderr, "Erasing %s failed on block %d\n", filename, i);
            break;
        }
    }
}

void holotape_wrapper::extract(const char *target_directory)
{
    if (!initialized)
    {
        fprintf(stderr, "Cannot extract the contents of %s as initialization failed\n", filename);
        return;
    }

    fprintf(stdout, "Extracting the contents of holotape %s:\n", filename);
    holotape_rewind(deck);

    // TODO: Duplicate file names may exist - must handle that
}

void holotape_wrapper::list()
{
    if (!initialized)
    {
        fprintf(stderr, "Cannot list the contents of %s as initialization failed\n", filename);
        return;
    }

    fprintf(stdout, "Listing contents of holotape %s:\n", filename);
    holotape_rewind(deck);

    char name[HOLOTAPE_FILE_NAME_MAX + 1];
    name[HOLOTAPE_FILE_NAME_MAX] = 0;
    int file_count = 0;
    for (int i = 0; i < HOLOTAPE_MAX_BLOCKS; i++)
    {
        auto read_status = holotape_read(deck);
        if (read_status == HOLO_NO_ERROR)
        {
            // Print the name of the last block of the file. If multiple files with the same name are found,
            // it'll print those all out
            if (deck->block_buffer.block_structure.remaining_blocks_in_file == 0 && deck->block_buffer.block_structure.filename[0] != 0)
            {
                strncpy(name, deck->block_buffer.block_structure.filename, HOLOTAPE_FILE_NAME_MAX);
                fprintf(stdout, "- %s\n", name);
                file_count++;
            }
        }
        else
        {
            fprintf(stderr, "Read failed when listing the contents of holotape %s\n", filename);
            return;
        }
    }

    if (file_count == 0)
    {
        fprintf(stdout, "\nNo files on holotape\n");
    }
}
