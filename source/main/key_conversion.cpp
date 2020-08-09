#include "key_conversion.h"

const char *shifted_nums = ")!@#$%^&*(";
const char *shifted_keys = " !\"#$%&\"()*+<_>?)!@#$%^&*(::<+>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}^_~ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~";

int sdl_keycode_to_console_key(SDL_Keysym &keysym)
{
    int console_key = 0;
    bool has_shift = keysym.mod & KMOD_LSHIFT || keysym.mod & KMOD_RSHIFT;
    if (keysym.sym >= 0 && keysym.sym < 0x7F)
    {
        if (has_shift)
        {
            int index = keysym.sym - ' ';
            console_key = shifted_keys[index];
        }
        else
        {
            console_key = keysym.sym;
        }
    }
    else
    {
        switch (keysym.sym)
        {
        case SDLK_HOME:
            console_key = KEY_HOME;
            break;

        case SDLK_END:
            console_key = KEY_END;
            break;

        case SDLK_LEFT:
            console_key = KEY_LEFT_ARROW;
            break;

        case SDLK_RIGHT:
            console_key = KEY_RIGHT_ARROW;
            break;

        case SDLK_DOWN:
            console_key = KEY_DOWN_ARROW;
            break;

        case SDLK_UP:
            console_key = KEY_UP_ARROW;
            break;
        }
    }
 
    return console_key;
}
