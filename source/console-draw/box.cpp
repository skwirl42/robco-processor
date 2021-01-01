#include "box.hpp"

box::box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char)
    : type(type), fill(fill), x(x), y(y), width(width), height(height), fill_char(fill_char)
{

}

void box::draw(drawer *drawer)
{
    drawer->draw_box(type, fill, x, y, width, height, fill_char);
}