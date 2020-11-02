#include "holotape.h"

#include <stdio.h>

typedef struct _holotape_state
{
    char holotape_filename[HOLOTAPE_BLOCK_SIZE+1];
    FILE *current_holotape_file;
    int current_block;
} holotape_state_t;