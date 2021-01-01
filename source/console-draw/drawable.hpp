#pragma once

#include "drawer.hpp"

class drawable
{
public:
    virtual void draw(drawer *drawer) = 0;
};