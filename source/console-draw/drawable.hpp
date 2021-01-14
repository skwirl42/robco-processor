#pragma once

#include "drawer.hpp"
#include "rect.hpp"

class drawable
{
public:
    drawable(rect&bounds) : bounds(bounds) {}

    virtual void draw(drawer *drawer) = 0;

protected:
    rect bounds;
};