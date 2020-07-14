#include "key_conversion.h"

int sdl_keycode_to_console_key(SDL_Keysym &keysym)
{
    int console_key = 0;
    if (keysym.sym >= 'a' & keysym.sym <= 'z')
    {
        console_key = keysym.sym;
        if (keysym.mod & KMOD_LSHIFT || keysym.mod & KMOD_RSHIFT)
        {
            console_key = console_key & 0b1011111;
        }
    }
    else if (keysym.sym >= ' ' && keysym.sym <= 0x7F)
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
