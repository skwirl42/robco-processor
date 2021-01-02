#pragma once

enum class button_event
{
    // nothing happened
    none,

    // the button was "clicked", most likely the enter key
    clicked,

    // when the button gets keyboard focus
    focused,
};

enum class text_field_event
{
    none,
    enter_pressed,
    text_updated,
    focused,
};

enum class text_event_send_mode
{
    on_enter,
    on_changed,
};

enum class box_type
{
    none,
    single_line,
    double_line,
    block,
};

enum class fill_mode
{
    none,
    clear,
    character,
};
