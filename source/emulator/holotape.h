#ifndef __HOLOTAPE_H__
#define __HOLOTAPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "emulator.h"

#define HOLOTAPE_BLOCK_SIZE         1024
#define HOLOTAPE_FILE_NAME_MAX      8

typedef enum _holotape_status
{
    HOLO_NO_ERROR,
    HOLO_EMPTY,

    HOLO_NOT_FOUND,
    HOLO_NAME_TOO_LONG,
} holotape_status_t;

typedef struct _holotape_state holotape_state_t;

typedef struct _holotape_block
{
    emulator_word_t block_bytes;
    emulator_word_t remaining_blocks_in_file;
    char *filename[HOLOTAPE_FILE_NAME_MAX];
    uint8_t bytes[HOLOTAPE_BLOCK_SIZE - HOLOTAPE_FILE_NAME_MAX - (2 * sizeof(emulator_word_t))];
} holotape_block_t;

typedef union _holotape_block_buffer
{
    holotape_block_t    block_structure;
    uint8_t             buffer[HOLOTAPE_BLOCK_SIZE];
} holotape_block_buffer_t;

typedef struct _holotape_deck
{
    holotape_state_t *current_holotape;
    holotape_block_buffer_t block_buffer;
} holotape_deck_t;

holotape_deck_t *holotape_deck_init();
void holotape_deck_dispose(holotape_deck_t *deck);

// Any block read/write functions will read/write from/to the block_buffer of the deck's struct
holotape_status_t holotape_check(holotape_deck_t *deck);
holotape_status_t holotape_eject(holotape_deck_t *deck);
holotape_status_t holotape_rewind(holotape_deck_t *deck);
holotape_status_t holotape_find(holotape_deck_t *deck, const char *holo_filename);
holotape_status_t holotape_execute(holotape_deck_t *deck);
holotape_status_t holotape_seek(holotape_deck_t *deck, uint16_t seek_blocks);
holotape_status_t holotape_read(holotape_deck_t *deck, uint16_t *out_byte_count);
holotape_status_t holotape_write(holotape_deck_t *deck, uint16_t byte_count);
holotape_status_t holotape_append(holotape_deck_t *deck, uint8_t *bytes, uint16_t byte_count);
holotape_status_t holotape_blocks(holotape_deck_t *deck, uint16_t *out_block_count);
holotape_status_t holotape_blockbytes(holotape_deck_t *deck, uint16_t *out_byte_count);

#ifdef __cplusplus
}
#endif

#endif // __HOLOTAPE_H__