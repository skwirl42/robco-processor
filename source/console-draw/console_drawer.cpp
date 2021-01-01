#include "console_drawer.hpp"

#include <string.h>

#include "text_field.hpp"

namespace
{
    inline int min(int a, int b)
    {
        return a > b ? b : a;
    }

    inline int max(int a, int b)
    {
        return a < b ? b : a;
    }

    const int box_top_left_index = 0;
    const int box_horizontal_index = 1;
    const int box_top_right_index = 2;
    const int box_vertical_index = 3;
    const int box_bottom_left_index = 4;
    const int box_bottom_right_index = 5;

    const char single_line_box_characters[] = {
        '\xDA', // top-left corner
        '\xC4', // horizontal line
        '\xBF', // top-right corner
        '\xB3', // vertical line
        '\xC0', // bottom-left
        '\xD9', // bottom-right
    };

    const char double_line_box_characters[] = {
        '\xC9', // top-left corner
        '\xCD', // horizontal line
        '\xBB', // top-right corner
        '\xBA', // vertical line
        '\xC8', // bottom-left
        '\xBC', // bottom-right
    };

    const char block_box_characters[] = {
        '\xDB', // top-left corner
        '\xDB', // horizontal line
        '\xDB', // top-right corner
        '\xDB', // vertical line
        '\xDB', // bottom-left
        '\xDB', // bottom-right
    };
} // namespace

console_drawer::console_drawer(Console &target_console)
    : target_console(target_console), width(target_console.GetWidth()), height(target_console.GetHeight()), next_control_id(0),
      focused_control_index(-1), cursor_enabled(false)
{

}

console_drawer::~console_drawer()
{
    controls.clear();
}

void console_drawer::handle_key(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_TAB:
    case SDLK_RIGHT:
    case SDLK_LEFT:
    case SDLK_UP:
    case SDLK_DOWN:
        if (controls.size() > 1)
        {
            int last_focused_control = focused_control_index;
            if (key == SDLK_LEFT || key == SDLK_UP)
            {
                focused_control_index--;
                if (focused_control_index < 0)
                {
                    focused_control_index = controls.size() - 1;
                }
            }
            else
            {
                focused_control_index++;
                if (focused_control_index >= controls.size())
                {
                    focused_control_index = 0;
                }
            }

            controls[last_focused_control]
                .get()
                ->set_focused(false);
            controls[focused_control_index].get()->set_focused(true);
        }
        break;

    default:
        if (controls[focused_control_index].get()->wants_keys())
        {
            bool handled = controls[focused_control_index].get()->handle_key(key);
            if (!handled)
            {
                // Should there be some sort of fallback for key handling?
            }
        }
        break;
    }
}

void console_drawer::draw()
{
    draw_boxes();
    draw_controls();
}

void console_drawer::draw_boxes()
{
    for (auto &box : boxes)
    {
        box.draw(this);
    }
}

void console_drawer::draw_controls()
{
    int cursor_x = -1;
    int cursor_y = -1;
    cursor_enabled = false;
    for (auto &control : controls)
    {
        auto control_ptr = control.get();
        control_ptr->draw(this);

        auto text = dynamic_cast<text_field *>(control_ptr);
        if (control_ptr->is_focused() && text != nullptr)
        {
            cursor_enabled = true;
            cursor_x = text->get_field_start() + text->get_cursor_position();
            cursor_y = text->get_y();
        }
    }

    if (cursor_x >= 0 && cursor_y >= 0)
    {
        target_console.SetCursor(cursor_x, cursor_y);
    }
}

void console_drawer::draw_box(box_type type, fill_mode fill, int xin, int yin, int width, int height, char fill_char)
{
    target_console.SetCurrentAttribute(CharacterAttribute::None);
    int right_edge = xin + width - 1;
    int bottom_edge = yin + height - 1;

    for (int y = yin; y < yin + height; y++)
    {
        for (int x = xin; x < xin + width; x++)
        {
            const char *chosen_box_chars = nullptr;
            switch (type)
            {
            case box_type::block:
                chosen_box_chars = block_box_characters;
                break;

            case box_type::double_line:
                chosen_box_chars = double_line_box_characters;
                break;

            case box_type::single_line:
                chosen_box_chars = single_line_box_characters;
                break;

            case box_type::none:
                break;
            }
            bool at_left = x == xin;
            bool at_top = y == yin;
            bool at_right = x == right_edge;
            bool at_bottom = y == bottom_edge;
            bool at_edge = at_left || at_top || at_right || at_bottom;
            if (at_edge && chosen_box_chars)
            {
                char chosen_char = fill_char;
                if ((at_top || at_bottom) && !(at_left || at_right))
                {
                    // Horizontal line
                    chosen_char = chosen_box_chars[box_horizontal_index];
                }
                else if (!(at_top || at_bottom) && (at_left || at_right))
                {
                    // Vertical line
                    chosen_char = chosen_box_chars[box_vertical_index];
                }
                else if (at_top && at_left)
                {
                    chosen_char = chosen_box_chars[box_top_left_index];
                }
                else if (at_top && at_right)
                {
                    chosen_char = chosen_box_chars[box_top_right_index];
                }
                else if (at_bottom && at_left)
                {
                    chosen_char = chosen_box_chars[box_bottom_left_index];
                }
                else if (at_bottom && at_right)
                {
                    chosen_char = chosen_box_chars[box_bottom_right_index];
                }
                target_console.SetChar(x, y, chosen_char);
            }
            else
            {
                switch (fill)
                {
                case fill_mode::character:
                    target_console.SetChar(x, y, fill_char);
                    break;

                case fill_mode::clear:
                    target_console.SetChar(x, y, ' ');
                    break;

                case fill_mode::none:
                    break;
                }
            }
        }
    }
}

void console_drawer::set_rect(char set_char, int xin, int yin, int width, int height)
{
    if (xin > this->width || yin > this->height)
    {
        return;
    }

    target_console.SetCurrentAttribute(CharacterAttribute::None);

    if (xin < 0)
    {
        xin = 0;
    }

    if (yin < 0)
    {
        yin = 0;
    }

    auto right_side = xin + min(this->width, width);
    auto bottom_side = yin + min(this->height, height);

    for (int y = yin; y < bottom_side; y++)
    {
        for (int x = xin; x < right_side; x++)
        {
            target_console.SetChar(x, y, set_char);
        }
    }
}

void console_drawer::draw_text(const char *text, int x, int y, bool inverted)
{
    target_console.SetCurrentAttribute(inverted ? CharacterAttribute::Inverted : CharacterAttribute::None);
    target_console.PrintAt(text, x, y);
}

int console_drawer::add_control(control *new_control)
{
    int index = controls.size();

    bool has_focused_control = false;
    for (auto &control : controls)
    {
        if (control.get()->is_focused())
        {
            has_focused_control = true;
            break;
        }
    }

    if (!has_focused_control)
    {
        focused_control_index = index;
        new_control->set_focused(true);
    }
    controls.push_back(std::unique_ptr<control>(new_control));

    return next_control_id++;
}

int console_drawer::define_button(const char *text, int x, int y, int width, int height, button_handler handler)
{
    return add_control(new button(text, handler, next_control_id, x, y, width, height, false));
}

int console_drawer::define_text_field(const char *label_text, text_field_event_handler handler, text_event_send_mode send_mode, int x, int y, int max_content_length, const char *initial_contents)
{
    return add_control(new text_field(next_control_id, label_text, handler, send_mode, x, y, max_content_length, initial_contents, false));
}

void console_drawer::remove_control_by_id(int id)
{
    bool has_focus = false;
    auto iterator = controls.begin();
    for (int i = 0; i < controls.size() && iterator != controls.end(); i++, iterator++)
    {
        auto control = controls[i].get();
        if (control->get_id() == id)
        {
            has_focus = control->is_focused();
            controls.erase(iterator);
            if (has_focus)
            {
                if (i < controls.size())
                {
                    controls[i].get()->set_focused(true);
                }
                else if (controls.size() > 0)
                {
                    controls[i - 1].get()->set_focused(true);
                    focused_control_index = i - 1;
                }
            }
            break;
        }
    }

    if (!has_focus)
    {
        for (int i = 0; i < controls.size(); i++)
        {
            if (controls[i].get()->is_focused())
            {
                focused_control_index = i;
                break;
            }
        }
    }
}

void console_drawer::add_box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char)
{
    boxes.push_back(box(type, fill, x, y, width, height, fill_char));
}