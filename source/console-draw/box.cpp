#include "box.hpp"

box::box(int id, box_type type, fill_mode fill, const rect& bounds, char fill_char)
    : type(type), fill(fill), id(id), fill_char(fill_char), drawable(bounds)
{

}

void box::draw(drawer *drawer)
{
    drawer->draw_box(type, fill, bounds, fill_char);
}