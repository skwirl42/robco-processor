#pragma once

#include "enums.hpp"
#include "Console.h"
#include "rect.hpp"

class drawer
{
public:
    virtual void draw_box(box_type type, fill_mode fill, const rect& bounds, char fill_char = 0) = 0;
    virtual void set_rect(char set_char, const rect& bounds) = 0;
    virtual void draw_text(const char *text, int x, int y, CharacterAttribute attribute) = 0;
};