#pragma once

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

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
    KEY_HOME            = KEY_FUNC_5 + 1,
    KEY_END             = KEY_HOME + 1,
    KEY_PAGE_UP         = KEY_HOME + 2,
    KEY_PAGE_DOWN       = KEY_HOME + 3,
    KEY_ALT             = KEY_PAGE_DOWN + 1,
} keycode_t;

int sdl_keycode_to_console_key(SDL_Keysym &keysym);
