#pragma once

#include "drawer.hpp"

class drawable
{
public:
    drawable(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}

    virtual void draw(drawer *drawer) = 0;

protected:
    int x;
    int y;
    int width;
    int height;
};