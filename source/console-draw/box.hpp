#pragma once

#include "enums.hpp"
#include "drawable.hpp"

class box : public drawable
{
public:
    box(int id, box_type type, fill_mode fill, const rect& bounds, char fill_char = ' ');

    virtual void draw(drawer *drawer);

    int get_id() const { return id; }

private:
    box_type type;
    fill_mode fill;
    int id;
    char fill_char;
};