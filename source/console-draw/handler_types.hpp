#pragma once

/// <summary>
/// The bare minimum data for any handler is the event enum and the ID of the control.
/// </summary>

#include "enums.hpp"

typedef std::function<void(focus_event, int)> focus_event_handler;
typedef std::function<void(text_field_event, int, const char*)> text_field_event_handler;
typedef std::function<void(button_event, int)> button_handler;
