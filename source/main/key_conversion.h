#pragma once

#include <SDL2/SDL.h>

typedef enum
{
    KEY_LEFT_ARROW      = 256,
    KEY_RIGHT_ARROW     = KEY_LEFT_ARROW + 1,
    KEY_UP_ARROW        = KEY_LEFT_ARROW + 2,
    KEY_DOWN_ARROW      = KEY_LEFT_ARROW + 3,
    KEY_FUNC_0          = KEY_LEFT_ARROW + 4,
    KEY_FUNC_1          = KEY_FUNC_0 + 1,
    KEY_FUNC_2          = KEY_FUNC_0 + 2,
    KEY_FUNC_3          = KEY_FUNC_0 + 3,
    KEY_FUNC_4          = KEY_FUNC_0 + 4,
    KEY_FUNC_5          = KEY_FUNC_0 + 5,
    KEY_FUNC_6          = KEY_FUNC_0 + 6,
    KEY_FUNC_7          = KEY_FUNC_0 + 7,
    KEY_FUNC_8          = KEY_FUNC_0 + 8,
    KEY_FUNC_9          = KEY_FUNC_0 + 9,
    KEY_FUNC_10         = KEY_FUNC_0 + 10,
    KEY_FUNC_11         = KEY_FUNC_0 + 11,
    KEY_FUNC_12         = KEY_FUNC_0 + 12,
    KEY_FUNC_13         = KEY_FUNC_0 + 13,
    KEY_FUNC_14         = KEY_FUNC_0 + 14,
    KEY_HOME            = KEY_FUNC_14 + 1,
    KEY_END             = KEY_HOME + 1,
    KEY_PAGE_UP         = KEY_HOME + 2,
    KEY_PAGE_DOWN       = KEY_HOME + 3,
} keycode_t;

int sdl_keycode_to_console_key(SDL_Keysym &keysym);
