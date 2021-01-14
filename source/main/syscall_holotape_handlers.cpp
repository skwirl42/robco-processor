#include "syscall_holotape_handlers.h"

#include <string.h>
#include <memory>

#include "holotape.h"
#include "syscall.h"
#include "executable_file.h"
#include "exceptions.hpp"

namespace
{
    holotape_deck_t *current_deck = nullptr;
}

void handle_holotape_find(emulator &emulator)
{
    const char *filename = reinterpret_cast<const char *>(&emulator.memories.data[emulator.X]);
    auto result = holotape_find(current_deck, filename);
    push_word(&emulator, result);
}

void handle_holotape_execute(emulator &emulator)
{
    auto check_result = holotape_check(current_deck);
    if (check_result != HOLO_NOT_EMPTY && check_result != HOLO_NO_ERROR)
    {
        push_byte(&emulator, check_result);
        return;
    }

    // This is gonna be a slightly bigger job than the other holotape functions...
    holotape_status_t result;
    if ((result = holotape_read(current_deck)) != HOLO_NO_ERROR)
    {
        push_byte(&emulator, result);
        return;
    }

    int block_count = current_deck->block_buffer.block_structure.remaining_blocks_in_file.word;
    executable_file_header_t header{};
    memcpy(&header, current_deck->block_buffer.block_structure.bytes, sizeof(executable_file_header_t));

    if (header.segment_count == 0 || header.total_length == 0)
    {
        push_byte(&emulator, HOLO_EXEC_FORMAT_ERROR);
        return;
    }

    std::unique_ptr<uint8_t[]> executable_file(new uint8_t[header.total_length]);
    auto remaining_length = header.total_length;
    auto bytes_size = current_deck->block_buffer.block_structure.block_bytes.word - HOLOTAPE_HEADER_SIZE;
    memcpy(executable_file.get(), current_deck->block_buffer.block_structure.bytes, bytes_size);
    if (bytes_size > remaining_length)
    {
        // It's possible that the file size of an executable is greater than the actual executable data
        // Is it the EOF from the original host OS file?
        remaining_length = 0;
    }
    else
    {
        remaining_length -= bytes_size;
    }

    while (remaining_length > 0)
    {
        if ((result = holotape_read(current_deck)) != HOLO_NO_ERROR)
        {
            push_byte(&emulator, result);
            return;
        }

        auto size_to_read = current_deck->block_buffer.block_structure.block_bytes.word - HOLOTAPE_HEADER_SIZE;
        if (size_to_read < remaining_length)
        {
            push_byte(&emulator, HOLO_EXEC_UNEXPECTED_END_OF_FILE);
            return;
        }

        size_to_read = size_to_read > remaining_length ? remaining_length : size_to_read;

        memcpy(&(executable_file.get()[remaining_length]), current_deck->block_buffer.block_structure.bytes, size_to_read);

        remaining_length -= size_to_read;
    }

    std::unique_ptr<uint8_t[]> prepared_memory(new uint8_t[DATA_SIZE]);
    memset(prepared_memory.get(), 0, DATA_SIZE);

    executable_segment_header_t current_segment_header{};
    auto current_file_position = sizeof(executable_file_header_t);
    while (current_file_position < header.total_length)
    {
        // Grab the header
        memcpy(&current_segment_header, &(executable_file.get()[current_file_position]), EXEC_SEGMENT_HEADER_RAW_SIZE);
        if ((current_file_position + current_segment_header.segment_length) > DATA_SIZE)
        {
            throw basic_error() << error_message("A segment went past the end of memory");
        }

        current_file_position += EXEC_SEGMENT_HEADER_RAW_SIZE;

        // How much do we have left to copy?
        auto data_length = current_segment_header.segment_length - EXEC_SEGMENT_HEADER_RAW_SIZE;
        memcpy(&(prepared_memory.get()[current_segment_header.segment_location]), &(executable_file.get()[current_file_position]), data_length);
        current_file_position += data_length;
    }

    // Execution itself is no big deal, really, just clear out the emulator, reset
    // its registers, etc, and then set the PC to the executable's start address
    reset_emulator(&emulator);
    emulator.PC = header.execution_start_address;
    emulator.current_state = RUNNING;
    memcpy(emulator.memories.data, prepared_memory.get(), DATA_SIZE);
    holotape_rewind(current_deck);
}

void handle_holotape_read(emulator &emulator)
{
    auto result = holotape_read(current_deck);
    if (result == HOLO_NO_ERROR)
    {
        uint8_t *buffer = &emulator.memories.data[emulator.X];
        memcpy(buffer, current_deck->block_buffer.buffer, HOLOTAPE_BLOCK_SIZE);
    }
    
    push_word(&emulator, result);
}

void handle_holotape_write(emulator &emulator)
{
    uint8_t *buffer = &emulator.memories.data[emulator.X];
    memcpy(current_deck->block_buffer.buffer, buffer, HOLOTAPE_BLOCK_SIZE);
    push_word(&emulator, holotape_write(current_deck));
}

void handle_holotape_syscall(emulator &emulator)
{
    if (current_deck == nullptr)
    {
        current_deck = holotape_deck_init();
    }

    switch (emulator.current_syscall)
    {
    case SYSCALL_HOLOTAPECHECK:
        if (holotape_check(current_deck) == HOLO_NOT_EMPTY)
        {
            push_byte(&emulator, 1);
        }
        else
        {
            push_byte(&emulator, 0);
        }
        break;

    case SYSCALL_HOLOTAPEEJECT:
        push_word(&emulator, holotape_eject(current_deck));
        break;
        
    case SYSCALL_REWIND:
        push_word(&emulator, holotape_rewind(current_deck));
        break;
        
    case SYSCALL_SEEK:
        push_word(&emulator, holotape_seek(current_deck, pull_word(&emulator)));
        break;
        
    case SYSCALL_FIND:
        handle_holotape_find(emulator);
        break;
        
    case SYSCALL_EXECUTE:
        handle_holotape_execute(emulator);
        break;
        
    case SYSCALL_READ:
        handle_holotape_read(emulator);
        break;
        
    case SYSCALL_WRITE:
        handle_holotape_write(emulator);
        break;
    }
}

void insert_holotape(const char *holotape_file)
{
    if (current_deck == nullptr)
    {
        current_deck = holotape_deck_init();
    }

    auto result = holotape_insert(current_deck, holotape_file);
    if (result != HOLO_NO_ERROR)
    {
        throw basic_error() << error_message("Couldn't insert the holotape");
    }
}

void eject_holotape()
{
    if (current_deck == nullptr)
    {
        current_deck = holotape_deck_init();
    }

    if (current_deck->current_holotape != nullptr)
    {
        holotape_eject(current_deck);
    }
}

bool holotape_initialized()
{
    return current_deck != nullptr;
}

void dispose_holotape()
{
    holotape_deck_dispose(current_deck);
    current_deck = nullptr;
}