#pragma once

enum class focus_event
{
    none,
    gained_focus,
    lost_focus,
};

enum class button_event
{
    none,
    clicked,
    focused,
    lost_focus,
};

enum class text_field_event
{
    none,
    enter_pressed,
    text_updated,
    focused,
    lost_focus,
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
