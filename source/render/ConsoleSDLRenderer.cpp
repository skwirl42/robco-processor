#include "ConsoleSDLRenderer.h"

#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <stdio.h>

ConsoleSDLRenderer::ConsoleSDLRenderer(const char *fontFilename, int width, int height, uint32_t foregroundColour, uint32_t backgroundColour, int cursorBlinkFrames)
    : width(width), height(height), foregroundColour(foregroundColour), backgroundColour(backgroundColour), cursorBlinkFrames(cursorBlinkFrames), isValid(false),
      window(nullptr), renderer(nullptr), texture(nullptr), fontBuffer(nullptr)
{
    auto result = SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_SHOWN, &window, &renderer);
    if (result != 0)
    {
        fprintf(stderr, "Failed to create SDL window and renderer (%s)", SDL_GetError());
        Cleanup();
        return;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == nullptr)
    {
        fprintf(stderr, "Failed to create SDL texture (%s)", SDL_GetError());
        Cleanup();
        return;
    }

    auto formatsFlags = IMG_Init(IMG_INIT_PNG);

    if ((formatsFlags & IMG_INIT_PNG) == 0)
    {
        fprintf(stderr, "Failed to initialize image loading %s", IMG_GetError());
        Cleanup();
        return;
    }

    auto fontSurface = IMG_Load(fontFilename);
    if (fontSurface == nullptr)
    {
        fprintf(stderr, "Failed to load image (%s), error: (%s)", fontFilename, IMG_GetError());
        Cleanup();
        return;
    }

    bool fontSuccess = true;

    fontBuffer = new bool[fontSurface->w * fontSurface->h];

    SDL_LockSurface(fontSurface);

    auto fontSurfaceFormat = fontSurface->format->format;
    if (SDL_BITSPERPIXEL(fontSurfaceFormat) == 32 && SDL_ISPIXELFORMAT_PACKED(fontSurfaceFormat))
    {
        
    }
    else
    {
        fprintf(stderr, "Couldn't handle the font buffer's format (0x%x)", fontSurfaceFormat);
        fontSuccess = false;
    }

    SDL_UnlockSurface(fontSurface);

    if (!fontSuccess)
    {
        Cleanup();
        if (fontBuffer != nullptr)
        {
            delete [] fontBuffer;
            fontBuffer = nullptr;
        }
        if (fontSurface != nullptr)
        {
            SDL_FreeSurface(fontSurface);
        }
        return;
    }

    isValid = true;
}

void ConsoleSDLRenderer::Cleanup()
{
    isValid = false;

    if (renderer != nullptr)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    
    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    if (texture != nullptr)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

void ConsoleSDLRenderer::Clear()
{

}

void ConsoleSDLRenderer::SetColours(uint32_t foregroundColour, uint32_t backgroundColour)
{

}

void ConsoleSDLRenderer::Render(Console *console, int frame)
{

}
