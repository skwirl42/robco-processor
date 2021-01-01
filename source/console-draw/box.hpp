#pragma once

#include "enums.hpp"
#include "drawable.hpp"

class box : public drawable
{
public:
    box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char = ' ');

    virtual void draw(drawer *drawer);

private:
    box_type type;
    fill_mode fill;
    int x;
    int y;
    int width;
    int height;
    char fill_char;
};