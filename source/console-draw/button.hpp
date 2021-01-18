#pragma once

#include <functional>

#include "enums.hpp"
#include "control.hpp"
#include "handler_types.hpp"

class button : public control
{
public:
    button(const char *text, button_handler handler, int id, const rect& bounds, bool focused);

    virtual void draw(drawer *drawer);

    void send_event(button_event event);

    void set_text(const char *new_text, bool resize = true);

    virtual bool wants_keys() const { return true; }
    virtual bool handle_key(SDL_Keycode key);

protected:
    virtual void handle_focused(bool was_focused);

private:
    button_handler handler;
    const char *text;
    int x;
    int y;
    int width;
    int height;
};
