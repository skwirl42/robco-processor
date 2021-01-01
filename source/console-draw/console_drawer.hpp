#pragma once

#include "Console.h"

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

#include <functional>
#include <vector>
#include <memory>

#include "enums.hpp"
#include "focusable.hpp"
#include "drawer.hpp"
#include "button.hpp"
#include "box.hpp"
#include "text_field.hpp"

class console_drawer : public drawer
{
public:
    console_drawer(Console &target_console);
    ~console_drawer();

    void handle_key(SDL_Keycode key);

    void draw();

    void draw_boxes();
    void draw_controls();

    virtual void draw_box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char = 0);
    virtual void set_rect(char set_char, int x, int y, int width, int height);
    virtual void draw_text(const char *text, int x, int y, bool inverted);

    int define_button(const char *text, int x, int y, int width, int height, button_handler handler);
    int define_text_field(const char *label_text, text_field_event_handler handler, text_event_send_mode send_mode, int x, int y, int max_content_length, const char *initial_contents);
   
    void remove_control_by_id(int id);

    void add_box(box_type type, fill_mode fill, int x, int y, int width, int height, char fill_char = 0);
    int box_count() const { return boxes.size(); }
    void clear_boxes() { boxes.clear(); }

    bool is_cursor_enabled() const { return cursor_enabled; }

protected:
    int add_control(control *control);

private:
    std::vector<box> boxes;
    std::vector<std::unique_ptr<control>> controls;
    Console &target_console;
    int width;
    int height;
    int focused_control_index;
    int next_control_id;
    bool cursor_enabled;
};
