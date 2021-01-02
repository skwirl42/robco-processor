#pragma once

#include "focusable.hpp"
#include "drawable.hpp"

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

class control : public focusable, public drawable
{
public:
    control(int id, bool focused) : id(id), focusable(focused) {}
    virtual ~control() {}

    virtual bool wants_keys() const = 0;
    virtual bool handle_key(SDL_Keycode key) = 0;
    virtual int get_id() const { return id; };

protected:
    int id;
};