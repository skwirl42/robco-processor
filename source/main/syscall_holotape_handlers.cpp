#include "syscall_holotape_handlers.h"

#include <string.h>

#include "holotape.h"
#include "syscall.h"

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
    // TODO: This is gonna be a slightly bigger job than the others...
    // Need to read in the current block
    // Extract executable info from the read block
    // Figure out how many blocks need to be read for the executable size

    // Execution itself is no big deal, really, just clear out the emulator, reset
    // its registers, etc, and then set the PC to the executable's start address
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

void dispose_holotape()
{
    holotape_deck_dispose(current_deck);
    current_deck = nullptr;
}