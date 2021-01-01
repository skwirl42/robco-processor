#include "console_drawer.hpp"

#include <string.h>

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
    : target_console(target_console), width(target_console.GetWidth()), height(target_console.GetHeight()), next_button_id(0),
      focused_button_index(-1)
{

}

console_drawer::~console_drawer()
{
    buttons.clear();
}

void console_drawer::handle_key(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_TAB:
    case SDLK_RIGHT:
    case SDLK_LEFT:
        if (buttons.size() > 1)
        {
            int last_focused_button = focused_button_index;
            if (key == SDLK_LEFT)
            {
                focused_button_index--;
                if (focused_button_index < 0)
                {
                    focused_button_index = buttons.size() - 1;
                }
            }
            else
            {
                focused_button_index++;
                if (focused_button_index >= buttons.size())
                {
                    focused_button_index = 0;
                }
            }

            buttons[last_focused_button].set_focused(false);
            buttons[focused_button_index].set_focused(true);
        }
        break;

    default:
        if (buttons[focused_button_index].wants_keys())
        {
            bool handled = buttons[focused_button_index].handle_key(key);
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
    draw_buttons();
}

void console_drawer::draw_boxes()
{
    for (auto &box : boxes)
    {
        box.draw(this);
    }
}

void console_drawer::draw_buttons()
{
    target_console.SetCurrentAttribute(CharacterAttribute::None);
    for (auto &button : buttons)
    {
        button.draw(this);
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

int console_drawer::define_button(const char *text, int x, int y, int width, int height, button_handler handler)
{
    int index = buttons.size();

    bool has_focused_button = false;
    for (auto& button : buttons)
    {
        if (button.is_focused())
        {
            has_focused_button = true;
            break;
        }
    }

    if (!has_focused_button)
    {
        focused_button_index = index;
    }
    buttons.push_back(button(text, handler, next_button_id, x, y, width, height, !has_focused_button));

    return next_button_id++;
}

void console_drawer::remove_button_by_id(int id)
{
    bool has_focus = false;
    auto iterator = buttons.begin();
    for (int i = 0; i < buttons.size() && iterator != buttons.end(); i++, iterator++)
    {
        if (buttons[i].get_id() == id)
        {
            has_focus = buttons[i].is_focused();
            buttons.erase(iterator);
            if (has_focus)
            {
                if (i < buttons.size())
                {
                    buttons[i].set_focused(true);
                }
                else if (buttons.size() > 0)
                {
                    buttons[i - 1].set_focused(true);
                    focused_button_index = i - 1;
                }
            }
            break;
        }
    }

    if (!has_focus)
    {
        for (int i = 0; i < buttons.size(); i++)
        {
            if (buttons[i].is_focused())
            {
                focused_button_index = i;
                break;
            }
        }
    }
}

void console_drawer::add_box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char)
{
    boxes.push_back(box(type, fill, x, y, width, height, fill_char));
}