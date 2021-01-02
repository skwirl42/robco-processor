#pragma once

#include <functional>

#include "enums.hpp"
#include "control.hpp"

typedef std::function<void(button_event, int)> button_handler;

class button : public control
{
public:
    button(const char *text, button_handler handler, int id, int x, int y, int width, int height, bool focused);

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
