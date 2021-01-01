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
