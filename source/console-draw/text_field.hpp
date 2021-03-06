#pragma once

#include "control.hpp"
#include "handler_types.hpp"

#include <functional>

/// <summary>
/// A control displaying, and optionally allowing editing of, a single line of text along with an identifying label
/// </summary>
class text_field : public control
{
public:
    text_field(int id, const char *label_text, text_field_event_handler handler, text_event_send_mode send_mode, int x, int y, size_t max_content_length, const char *initial_contents, bool focused, bool editable);
    virtual ~text_field();

    void set_contents(const char *new_contents);
    int get_cursor_position() const { return cursor_position; }

    int get_x() const { return bounds.x; }
    int get_y() const { return bounds.y; }
    int get_field_start() const;

    virtual void draw(drawer *drawer);

    virtual bool wants_keys() const { return editable; }
    virtual bool handle_key(SDL_Keycode key);

    bool is_editable() const { return editable; }
    void set_editable(bool editable) { this->editable = editable; }

    int content_length() { return strlen(contents); }

    void send_event(text_field_event event);

protected:
    virtual void handle_focused(bool was_focused);

private:
    text_field_event_handler handler;
    const char *label_text;
    char *contents;
    size_t max_content_length;
    text_event_send_mode send_mode;
    int label_length;
    int cursor_position;
    bool editable;
};