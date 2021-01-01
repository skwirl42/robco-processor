#pragma once

#include "control.hpp"

#include <functional>

typedef std::function<void(text_field_event,int,const char *)> text_field_event_handler;

class text_field : public control
{
public:
    text_field(int id, const char *label_text, text_field_event_handler handler, text_event_send_mode send_mode, int x, int y, int max_content_length, const char *initial_contents, bool focused);
    virtual ~text_field();

    void set_contents(const char *new_contents);
    int get_cursor_position() const { return cursor_position; }

    virtual void draw(drawer *drawer);

    virtual bool wants_keys() const { return true; }
    virtual bool handle_key(SDL_Keycode key);

    int content_length() { return strlen(contents); }

    void send_event(text_field_event event);

protected:
    virtual void handle_focused();

private:
    text_field_event_handler handler;
    const char *label_text;
    char *contents;
    text_event_send_mode send_mode;
    int label_length;
    int x;
    int y;
    int width;
    int max_content_length;
    int cursor_position;
};