#pragma once

#include "enums.hpp"
#include "drawable.hpp"

class box : public drawable
{
public:
    box(box_type type, fill_mode fill, int id, int x, int y, int width, int height, char fill_char = ' ');

    virtual void draw(drawer *drawer);

    int get_id() const { return id; }

private:
    box_type type;
    fill_mode fill;
    int id;
    int x;
    int y;
    int width;
    int height;
    char fill_char;
};