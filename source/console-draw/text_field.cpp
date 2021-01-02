#include "text_field.hpp"

#include "key_conversion.h"

namespace
{
    const char empty_space_char = '\xB1';
}

text_field::text_field(int id, const char *label_text, text_field_event_handler handler, text_event_send_mode send_mode, int x, int y, int max_content_length, const char *initial_contents, bool focused, bool editable)
    : label_text(label_text), contents(new char[max_content_length + 1]), x(x), y(y), max_content_length(max_content_length),
      label_length(strlen(label_text)), width(x + strlen(label_text) + 1 + max_content_length), cursor_position(0), handler(handler), send_mode(send_mode), editable(editable),
      control(id, focused, true, x, y, strlen(label_text) + 1 + max_content_length, 1)
{
    memset(contents, 0, max_content_length);
    contents[max_content_length] = 0;

    if (strlen(initial_contents) > 0)
    {
        set_contents(initial_contents);
        cursor_position = strlen(initial_contents);
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

int text_field::get_field_start() const
{
    return x + label_length + 1;
}

void text_field::draw(drawer *drawer)
{
    drawer->draw_text(label_text, x, y, focused);
    drawer->draw_text(" ", x + label_length, y, focused);

    int box_start = get_field_start() + content_length();
    if (editable)
    {
        if (is_focused() && cursor_position == content_length())
        {
            // Clear a space for the cursor to blink
            drawer->draw_text(" ", get_field_start() + cursor_position, y, false);
            box_start++;
        }
    }

    if (content_length() > 0)
    {
        drawer->draw_text(contents, get_field_start(), y, !is_editable() && is_focused());
    }

    if (editable)
    {
        if (box_start < x + width)
        {
            int box_width = (x + width) - box_start;
            drawer->set_rect(empty_space_char, box_start, y, box_width, 1);
        }
    }
}

bool text_field::handle_key(SDL_Keycode key)
{
    if (!editable)
    {
        return false;
    }

    if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
    {
        send_event(text_field_event::enter_pressed);
        return true;
    }
    else if (key == SDLK_LEFT && cursor_position > 0)
    {
        cursor_position--;
        return true;
    }
    else if (key == SDLK_RIGHT && cursor_position < content_length())
    {
        cursor_position++;
        return true;
    }
    else if (key == SDLK_BACKSPACE && content_length() > 0 && cursor_position > 0)
    {
        int original_content_length = content_length();
        if (cursor_position < original_content_length)
        {
            for (int i = cursor_position - 1; i < original_content_length - 1; i++)
            {
                contents[i] = contents[i + 1];
            }
        }
        contents[original_content_length - 1] = 0;
        cursor_position--;
        if (send_mode == text_event_send_mode::on_changed)
        {
            send_event(text_field_event::text_updated);
        }
        return true;
    }
    else if (key == SDLK_DELETE && content_length() > 0 && cursor_position < content_length())
    {
        int original_content_length = content_length();
        if (cursor_position < original_content_length)
        {
            for (int i = cursor_position + 1; i < original_content_length; i++)
            {
                contents[i] = contents[i + 1];
            }
        }
        contents[original_content_length - 1] = 0;
        if (send_mode == text_event_send_mode::on_changed)
        {
            send_event(text_field_event::text_updated);
        }
        return true;
    }
    else if (key == SDLK_HOME)
    {
        cursor_position = 0;
    }
    else if (key == SDLK_END)
    {
        cursor_position = content_length();
    }
    else if (isprint(key) && content_length() < max_content_length)
    {
        int original_content_length = content_length();
        if (cursor_position < original_content_length)
        {
            for (int i = original_content_length; i > cursor_position; i--)
            {
                contents[i] = contents[i - 1];
            }
            contents[cursor_position] = (char)key;
        }
        else
        {
            contents[original_content_length] = (char)key;
        }
        
        cursor_position++;
        contents[original_content_length + 1] = 0;
        if (send_mode == text_event_send_mode::on_changed)
        {
            send_event(text_field_event::text_updated);
        }
        return true;
    }

    return false;
}

void text_field::handle_focused(bool was_focused)
{
    if (focused)
    {
        send_event(text_field_event::focused);
    }
    else if (was_focused)
    {
        send_event(text_field_event::lost_focus);
    }
}

void text_field::send_event(text_field_event event)
{
    handler(event, id, contents);
}
