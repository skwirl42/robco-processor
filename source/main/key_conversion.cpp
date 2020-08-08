#include "key_conversion.h"

const char *shifted_nums = ")!@#$%^&*(";

int sdl_keycode_to_console_key(SDL_Keysym &keysym)
{
    int console_key = 0;
    bool has_shift = keysym.mod & KMOD_LSHIFT || keysym.mod & KMOD_RSHIFT;
    if (keysym.sym >= 'a' && keysym.sym <= 'z')
    {
        console_key = keysym.sym;
        if (has_shift)
        {
            console_key = console_key & 0b1011111;
        }
    }
    else if (keysym.sym >= '0' && keysym.sym <= '9')
    {
        if (has_shift)
        {
            int index = keysym.sym - '0';
            console_key = shifted_nums[index];
        }
        else
        {
            console_key = keysym.sym;
        }
    }
    else if (keysym.sym >= 0 && keysym.sym <= 0x7F)
    {
        console_key = keysym.sym;
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
        }
    }
 
    return console_key;
}
