#pragma once

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)

enum class console_keycode
{
    KEY_SHIFT           = 0x80,
    KEY_CONTROL         = KEY_SHIFT + 1,
    KEY_ALT             = KEY_SHIFT + 2,
    KEY_META            = KEY_SHIFT + 3,

    KEY_LEFT_ARROW      = 256,
    KEY_RIGHT_ARROW     = KEY_LEFT_ARROW + 1,
    KEY_UP_ARROW        = KEY_LEFT_ARROW + 2,
    KEY_DOWN_ARROW      = KEY_LEFT_ARROW + 3,
    KEY_FUNC_0          = KEY_LEFT_ARROW + 4,
    KEY_FUNC_1          = KEY_FUNC_0 + 1,
    KEY_FUNC_2          = KEY_FUNC_0 + 2,
    KEY_FUNC_3          = KEY_FUNC_0 + 3,
    KEY_FUNC_4          = KEY_FUNC_0 + 4,
    KEY_HOME            = KEY_FUNC_4 + 1,
    KEY_END             = KEY_HOME + 1,
    KEY_PAGE_UP         = KEY_HOME + 2,
    KEY_PAGE_DOWN       = KEY_HOME + 3,
};

int sdl_keycode_to_console_key(SDL_Keysym &keysym);
int sdl_keycode_to_console_key(SDL_Keycode code, bool has_shift);
int sdl_scancode_to_console_key(SDL_Scancode code);
