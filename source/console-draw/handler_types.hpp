#pragma once

#include "enums.hpp"

class control;

template<typename T> class event_handler : public std::function<void(T, control*)> {};
typedef event_handler<focus_event> focus_event_handler;
typedef event_handler<button_event> button_handler;
typedef std::function<void(text_field_event, control*, const char*)> text_field_event_handler;
