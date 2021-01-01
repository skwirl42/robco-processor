#include "text_field.hpp"

#include "key_conversion.h"

namespace
{
    const char empty_space_char = '\xB1';
}

text_field::text_field(int id, const char *label_text, text_field_event_handler handler, text_event_send_mode send_mode, int x, int y, int max_content_length, const char *initial_contents, bool focused)
    : label_text(label_text), contents(new char[max_content_length + 1]), x(x), y(y), max_content_length(max_content_length),
      label_length(strlen(label_text)), width(x + strlen(label_text) + 1 + max_content_length), cursor_position(0), handler(handler), send_mode(send_mode),
      control(id, focused)
{
    memset(contents, 0, max_content_length);
    contents[max_content_length] = 0;

    if (strlen(initial_contents) > 0)
    {
        set_contents(initial_contents);
    }
}

text_field::~text_field()
{
    delete[] contents;
}

void text_field::set_contents(const char *new_contents)
{
    strncpy(contents, new_contents, max_content_length);
}

void text_field::draw(drawer *drawer)
{
    drawer->draw_text(label_text, x, y, focused);
    drawer->draw_text(" ", x + label_length, y, focused);

    int field_start = x + label_length + 1;
    int box_start = field_start + content_length();
    if (content_length() > 0)
    {
        drawer->draw_text(contents, field_start, y, false);
    }

    if (box_start < x + width)
    {
        int box_width = (x + width) - box_start;
        drawer->set_rect(empty_space_char, box_start, y, box_width, 1);
    }
}

bool text_field::handle_key(SDL_Keycode key)
{
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
    {
        send_event(text_field_event::enter_pressed);
        return true;
    }

    if (key == SDLK_BACKSPACE && content_length() > 0)
    {
        contents[content_length() - 1] = 0;
        cursor_position--;
        if (send_mode == text_event_send_mode::on_changed)
        {
            send_event(text_field_event::text_updated);
        }
        return true;
    }

    if (isprint(key) && content_length() < max_content_length)
    {
        contents[content_length()] = (char)key;
        cursor_position++;
        contents[cursor_position] = 0;
        if (send_mode == text_event_send_mode::on_changed)
        {
            send_event(text_field_event::text_updated);
        }
        return true;
    }

    return false;
}

void text_field::handle_focused()
{
    if (focused)
    {
        send_event(text_field_event::focused);
    }
}

void text_field::send_event(text_field_event event)
{
    handler(event, id, contents);
}
