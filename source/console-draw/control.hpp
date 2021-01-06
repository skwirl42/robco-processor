#pragma once

#include "focusable.hpp"
#include "drawable.hpp"
#include "rect.hpp"

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

class control : public focusable, public drawable
{
public:
    control(int id, bool focused, bool visible, rect& bounds) : id(id), visible(visible), focusable(focused), drawable(bounds) {}
    virtual ~control() {}

    virtual bool wants_keys() const = 0;
    virtual bool handle_key(SDL_Keycode key) = 0;
    virtual int get_id() const { return id; };

    virtual bool is_visible() const { return visible; }
    virtual void set_visible(bool visible) { this->visible = visible; }

protected:
    int id;
    bool visible;
};