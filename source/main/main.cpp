#include "ConsoleSDLRenderer.h"

#include <SDL2/SDL.h>
#include <stdio.h>

int main (int argc, char **argv)
{
    auto result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (result != 0)
    {
        fprintf(stderr, "Failed to initialize SDL (%s)", SDL_GetError());
        return -1;
    }

    ConsoleSDLRenderer *renderer = nullptr;
    if (argc > 1)
    {
        auto fontfilename = argv[1];
        renderer = new ConsoleSDLRenderer(fontfilename, 480, 320, 0x00FF00FF, 0x0000000FF, 60);
    }
    else
    {
        fprintf(stderr, "Missing argument: font filename");
    }

    if (renderer != nullptr)
    {
        // TODO: Loop!
    }

    if (renderer != nullptr)
    {
        delete renderer;
    }

    SDL_Quit();

    return 0;
}