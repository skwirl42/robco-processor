#include "holotape.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "opcodes.h"

typedef struct _holotape_state
{
    char holotape_filename[FILENAME_MAX+1];
    FILE *current_holotape_file;
    int current_block;
    size_t current_size;
} holotape_state_t;

static char empty_block_buffer[HOLOTAPE_BLOCK_SIZE];
static uint8_t initialized = 0;

holotape_deck_t *holotape_deck_init()
{
    holotape_deck_t *new_deck = (holotape_deck_t*)malloc(sizeof(holotape_deck_t));
    new_deck->current_holotape = 0;
    memset(new_deck->block_buffer.buffer, 0, sizeof(new_deck->block_buffer.buffer));
    return new_deck;
}

void holotape_deck_dispose(holotape_deck_t *deck)
{
    if (deck->current_holotape)
    {
        holotape_eject(deck);
    }
    free(deck);
}

holotape_status_t holotape_check(holotape_deck_t *deck)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (deck->current_holotape)
    {
        return HOLO_NOT_EMPTY;
    }

    return HOLO_EMPTY;
}

holotape_status_t holotape_eject(holotape_deck_t *deck)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (deck->current_holotape)
    {
        if (deck->current_holotape->current_holotape_file)
        {
            fclose(deck->current_holotape->current_holotape_file);
        }
        
        free(deck->current_holotape);
        deck->current_holotape = 0;
        return HOLO_NO_ERROR;
    }

    return HOLO_EMPTY;
}

holotape_status_t holotape_insert(holotape_deck_t *deck, const char *tape_filename)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (deck->current_holotape)
    {
        return HOLO_NOT_EMPTY;
    }

    if (strlen(tape_filename) > HOLOTAPE_BLOCK_SIZE)
    {
        return HOLO_TAPE_NAME_TOO_LONG;
    }

    deck->current_holotape = (holotape_state_t*)malloc(sizeof(holotape_state_t));
    memset(deck->current_holotape, 0, sizeof(holotape_state_t));

    FILE *tape_file = fopen(tape_filename, "a+");

    if (!tape_file)
    {
        free(deck->current_holotape);
        deck->current_holotape = 0;
        return HOLO_IO_ERROR;
    }

    deck->current_holotape->current_holotape_file = tape_file;
    strncpy(deck->current_holotape->holotape_filename, tape_filename, sizeof(FILENAME_MAX));

    if (!initialized)
    {
        memset(empty_block_buffer, 0, sizeof(empty_block_buffer));
        initialized = 1;
    }

    fseek(deck->current_holotape->current_holotape_file, 0, SEEK_END);
    deck->current_holotape->current_size = ftell(deck->current_holotape->current_holotape_file);

    holotape_status_t status = HOLO_NO_ERROR;
    if (deck->current_holotape->current_size == 0)
    {
        // Initialize a blank tape, since this is a new file
        for (int i = 0; i < HOLOTAPE_MAX_BLOCKS; i++)
        {
            size_t bytes_written = fwrite(empty_block_buffer, 1, HOLOTAPE_BLOCK_SIZE, deck->current_holotape->current_holotape_file);
            if (bytes_written < HOLOTAPE_BLOCK_SIZE)
            {
                status = HOLO_IO_ERROR;
                break;
            }
        }
        deck->current_holotape->current_size = ftell(deck->current_holotape->current_holotape_file);
    }

    fseek(deck->current_holotape->current_holotape_file, 0, SEEK_SET);

    return status;
}

holotape_status_t holotape_rewind(holotape_deck_t *deck)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (!deck->current_holotape)
    {
        return HOLO_EMPTY;
    }

    if (!deck->current_holotape->current_holotape_file)
    {
        return HOLO_IO_ERROR;
    }

    deck->current_holotape->current_block = 0;
    fseek(deck->current_holotape->current_holotape_file, 0, SEEK_SET);

    return HOLO_NO_ERROR;
}

holotape_status_t holotape_rewind_block(holotape_deck_t *deck)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (!deck->current_holotape)
    {
        return HOLO_EMPTY;
    }

    if (!deck->current_holotape->current_holotape_file)
    {
        return HOLO_IO_ERROR;
    }

    if (deck->current_holotape->current_block > 0)
    {
        deck->current_holotape->current_block--;
    }

    fseek(deck->current_holotape->current_holotape_file, deck->current_holotape->current_block * HOLOTAPE_BLOCK_SIZE, SEEK_SET);

    return HOLO_NO_ERROR;
}

holotape_status_t holotape_seek(holotape_deck_t *deck, uint16_t seek_blocks)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (!deck->current_holotape)
    {
        return HOLO_EMPTY;
    }

    if (!deck->current_holotape->current_holotape_file)
    {
        return HOLO_IO_ERROR;
    }

    size_t seek_to_blocks = deck->current_holotape->current_block + seek_blocks;
    size_t total_blocks_after = seek_to_blocks + 1;
    if (deck->current_holotape->current_size < (total_blocks_after * HOLOTAPE_BLOCK_SIZE))
    {
        fseek(deck->current_holotape->current_holotape_file, seek_to_blocks * HOLOTAPE_BLOCK_SIZE, SEEK_SET);
        size_t bytes_written = fwrite(empty_block_buffer, 1, HOLOTAPE_BLOCK_SIZE, deck->current_holotape->current_holotape_file);
        fseek(deck->current_holotape->current_holotape_file, -HOLOTAPE_BLOCK_SIZE, SEEK_END);
    }
    else
    {
        fseek(deck->current_holotape->current_holotape_file, seek_blocks * HOLOTAPE_BLOCK_SIZE, SEEK_CUR);
    }

    deck->current_holotape->current_block += seek_blocks;
    
    return HOLO_NO_ERROR;
}

holotape_status_t holotape_read(holotape_deck_t *deck)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (!deck->current_holotape)
    {
        return HOLO_EMPTY;
    }

    if (!deck->current_holotape->current_holotape_file)
    {
        return HOLO_IO_ERROR;
    }

    if (deck->current_holotape->current_block >= HOLOTAPE_MAX_BLOCKS)
    {
        return HOLO_END_OF_TAPE;
    }

    size_t bytes_read = fread(deck->block_buffer.buffer, 1, HOLOTAPE_BLOCK_SIZE, deck->current_holotape->current_holotape_file);
    if (bytes_read != HOLOTAPE_BLOCK_SIZE)
    {
        return HOLO_IO_ERROR;
    }

    deck->current_holotape->current_block++;

    return HOLO_NO_ERROR;
}

holotape_status_t holotape_write(holotape_deck_t *deck)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (!deck->current_holotape)
    {
        return HOLO_EMPTY;
    }

    if (!deck->current_holotape->current_holotape_file)
    {
        return HOLO_IO_ERROR;
    }

    size_t bytes_written = fwrite(deck->block_buffer.buffer, 1, HOLOTAPE_BLOCK_SIZE, deck->current_holotape->current_holotape_file);
    if (bytes_written != HOLOTAPE_BLOCK_SIZE)
    {
        return HOLO_IO_ERROR;
    }

    deck->current_holotape->current_block++;

    return HOLO_NO_ERROR;
}

holotape_status_t holotape_find(holotape_deck_t *deck, const char *holo_filename)
{
    if (!deck)
    {
        return HOLO_INVALID_DECK;
    }

    if (!deck->current_holotape)
    {
        return HOLO_EMPTY;
    }

    if (!deck->current_holotape->current_holotape_file)
    {
        return HOLO_IO_ERROR;
    }

    if (strlen(holo_filename) > HOLOTAPE_FILE_NAME_MAX)
    {
        return HOLO_NAME_TOO_LONG;
    }

    while (deck->current_holotape->current_block < HOLOTAPE_MAX_BLOCKS)
    {
        holotape_status_t status = holotape_read(deck);
        if (status != HOLO_NO_ERROR)
        {
            return status;
        }

        if (strncasecmp(holo_filename, deck->block_buffer.block_structure.filename, HOLOTAPE_FILE_NAME_MAX) == 0)
        {
            fseek(deck->current_holotape->current_holotape_file, -HOLOTAPE_BLOCK_SIZE, SEEK_CUR);
            return HOLO_NO_ERROR;
        }
    }

    return HOLO_NOT_FOUND;
}