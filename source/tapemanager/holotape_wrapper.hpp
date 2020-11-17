#pragma once

#include "holotape.h"

#define TAPE_NAME_LENGTH    1024

class holotape_wrapper
{
public:
    holotape_wrapper(const char *filename);
    ~holotape_wrapper();

    bool is_valid() const { return initialized; }

    void append_file(const char *file_to_append);
    void erase();
    void extract(const char *target_directory);
    void list();

private:
    const char *filename;
    holotape_deck_t *deck;
    bool initialized;
};