#include "button.hpp"

namespace
{
    const char focused_button_char = '\xDB';
    const char unfocused_button_char = '\xB1';
}

button::button(const char *text, button_handler handler, int id, int x, int y, int width, int height, bool focused)
    : text(text), handler(handler), id(id), x(x), y(y), width(width), height(height), control(focused)
{
}

void button::draw(drawer *drawer)
{
    int text_length = strlen(text);
    if (text_length < width || height > 1)
    {
        auto box_char = focused ? focused_button_char : unfocused_button_char;
        drawer->set_rect(box_char, x, y, width, height);
    }

    int half_x_position = x + (width / 2);
    int half_y_position = y + (height / 2);
    int text_x = half_x_position - (text_length / 2);
    drawer->draw_text(text, text_x, half_y_position, focused);
}

void button::send_event(button_event event, int extra_field)
{
    handler(event, id, extra_field);
}

void button::handle_focused()
{
    if (focused)
    {
        send_event(button_event::focused, id);
    }
}

int button::get_id() const
{
    return id;
}

bool button::handle_key(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_KP_ENTER:
    case SDLK_RETURN:
    case SDLK_SPACE:
        send_event(button_event::clicked);
        return true;
    }

    return false;
}
