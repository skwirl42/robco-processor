#include "key_conversion.h"

const char *shifted_nums = ")!@#$%^&*(";
const char *shifted_keys = " !\"#$%&\"()*+<_>?)!@#$%^&*(::<+>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}^_~ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~";

int sdl_scancode_to_console_key(SDL_Scancode code)
{
    int console_key = 0;
    auto keycode = SDL_GetKeyFromScancode(code);
    if (keycode >= 0 && keycode < 0x7F)
    {
        console_key = keycode;
    }
    else if (keycode >= SDLK_F1 && keycode <= SDLK_F4)
    {
        console_key = (int)console_keycode::KEY_FUNC_1 + (keycode - SDLK_F1);
    }
    else
    {
        switch (keycode)
        {
        case SDLK_HOME:
            console_key = (int)console_keycode::KEY_HOME;
            break;

        case SDLK_END:
            console_key = (int)console_keycode::KEY_END;
            break;

        case SDLK_LEFT:
            console_key = (int)console_keycode::KEY_LEFT_ARROW;
            break;

        case SDLK_RIGHT:
            console_key = (int)console_keycode::KEY_RIGHT_ARROW;
            break;

        case SDLK_DOWN:
            console_key = (int)console_keycode::KEY_DOWN_ARROW;
            break;

        case SDLK_UP:
            console_key = (int)console_keycode::KEY_UP_ARROW;
            break;

        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            console_key = (int)console_keycode::KEY_SHIFT;
            break;

        case SDLK_LCTRL:
        case SDLK_RCTRL:
            console_key = (int)console_keycode::KEY_CONTROL;
            break;

        case SDLK_LALT:
        case SDLK_RALT:
            console_key = (int)console_keycode::KEY_ALT;
            break;

        case SDLK_LGUI:
        case SDLK_RGUI:
            console_key = (int)console_keycode::KEY_META;
            break;
        }
    }
 
    return console_key;
}

int sdl_keycode_to_console_key(SDL_Keycode code, bool has_shift)
{
    int console_key = 0;
    if (code >= 0 && code < 0x7F)
    {
        if (has_shift)
        {
            int index = code - ' ';
            console_key = shifted_keys[index];
        }
        else
        {
            console_key = code;
        }
    }

    return console_key;
}

int sdl_keycode_to_console_key(SDL_Keysym &keysym)
{
    int console_key = 0;
    bool has_shift = keysym.mod & KMOD_LSHIFT || keysym.mod & KMOD_RSHIFT;

    console_key = sdl_keycode_to_console_key(keysym.sym, has_shift);

    if (console_key == 0)
    {
        console_key = sdl_scancode_to_console_key(keysym.scancode);
    }

    return console_key;
}
