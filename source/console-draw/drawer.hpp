#pragma once

#include "enums.hpp"

class drawer
{
public:
    virtual void draw_box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char = 0) = 0;
    virtual void set_rect(char set_char, int x, int y, int width, int height) = 0;
    virtual void draw_text(const char *text, int x, int y, bool inverted) = 0;
};