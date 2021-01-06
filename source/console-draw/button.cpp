#include "button.hpp"

namespace
{
    const char focused_button_char = '\xDB';
    const char unfocused_button_char = '\xB1';
}

button::button(const char *text, button_handler handler, int id, rect& bounds, bool focused)
    : text(text), handler(handler), x(x), y(y), width(width), height(height), control(id, focused, true, bounds)
{
}

void button::set_text(const char *new_text, bool resize)
{
    int text_len_delta = strlen(new_text) - strlen(text);
    text = new_text;
    if (resize)
    {
        width += text_len_delta;
    }
}

void button::draw(drawer *drawer)
{
    int text_length = strlen(text);
    if (text_length < width || height > 1)
    {
        auto box_char = focused ? focused_button_char : unfocused_button_char;
        drawer->set_rect(box_char, rect{ x, y, width, height });
    }

    int half_x_position = x + (width / 2);
    int half_y_position = y + (height / 2);
    int text_x = half_x_position - (text_length / 2);
    auto focusedAttribute = focused ? CharacterAttribute::Inverted : CharacterAttribute::None;
    drawer->draw_text(text, text_x, half_y_position, focusedAttribute);
}

void button::send_event(button_event event)
{
    handler(event, this);
}

void button::handle_focused(bool was_focused)
{
    if (focused)
    {
        send_event(button_event::focused);
    }
    else if (was_focused)
    {
        send_event(button_event::lost_focus);
    }
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
