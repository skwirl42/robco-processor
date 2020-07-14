#include "ConsoleSDLRenderer.h"

#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <stdio.h>

#include "Console.h"

ConsoleSDLRenderer::ConsoleSDLRenderer(const char *fontFilename, int width, int height, uint32_t foregroundColour, uint32_t backgroundColour, uint16_t fontCharsWide, uint16_t fontCharsHigh, int cursorBlinkFrames)
    : window(nullptr), renderer(nullptr), texture(nullptr), fontBuffer(nullptr),
      width(width), height(height), foregroundColour(foregroundColour), backgroundColour(backgroundColour),
      fontCharsWide(fontCharsWide), fontCharsHigh(fontCharsHigh),
      cursorBlinkFrames(cursorBlinkFrames),
      isValid(false)
{
    auto result = SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_SHOWN, &window, &renderer);
    if (result != 0)
    {
        fprintf(stderr, "Failed to create SDL window and renderer (%s)\n", SDL_GetError());
        Cleanup();
        return;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == nullptr)
    {
        fprintf(stderr, "Failed to create SDL texture (%s)\n", SDL_GetError());
        Cleanup();
        return;
    }

    auto formatsFlags = IMG_Init(IMG_INIT_PNG);

    if ((formatsFlags & IMG_INIT_PNG) == 0)
    {
        fprintf(stderr, "Failed to initialize image loading %s\n", IMG_GetError());
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
    if (SDL_PIXELTYPE(fontSurfaceFormat) == SDL_PIXELTYPE_INDEX8)
    {
        fontBufferWidth = fontSurface->w;
        fontBufferHeight = fontSurface->h;
        charPixelsWide = fontBufferWidth / fontCharsWide;
        charPixelsHigh = fontBufferHeight / fontCharsHigh;
        auto pixels = (uint8_t*)fontSurface->pixels;
        for (int y = 0; y < fontSurface->h; y++)
        {
            for (int x = 0; x < fontSurface->w; x++)
            {
                int surfaceIndex = (y * fontSurface->pitch + x);
                int fontIndex = (y * fontSurface->w + x);
                fontBuffer[fontIndex] = pixels[surfaceIndex] > 0;
            }
        }
    }
    else
    {
        unsigned int pixelType = SDL_PIXELTYPE(fontSurfaceFormat);
        unsigned int pixelLayout = SDL_PIXELLAYOUT(fontSurfaceFormat);
        fprintf(stderr, "Couldn't handle the font buffer's format (type: %u, layout: %u)\n", pixelType, pixelLayout);
        fontSuccess = false;
    }

    SDL_UnlockSurface(fontSurface);

    SDL_FreeSurface(fontSurface);

    if (!fontSuccess)
    {
        Cleanup();
        return;
    }

    isValid = true;
}

ConsoleSDLRenderer::~ConsoleSDLRenderer()
{
    Cleanup();
}

void ConsoleSDLRenderer::Cleanup()
{
    isValid = false;

    if (fontBuffer != nullptr)
    {
        delete [] fontBuffer;
        fontBuffer = nullptr;
    }

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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void ConsoleSDLRenderer::SetColours(uint32_t foregroundColour, uint32_t backgroundColour)
{

}

void ConsoleSDLRenderer::Render(Console *console, int frame)
{
    if (!isValid) return;

	bool cursorOn = false;
	if (cursorBlinkFrames > 0)
	{
		cursorOn = (frame / cursorBlinkFrames) % 2;
	}

	int cursorX;
	int cursorY;
	console->GetCursor(cursorX, cursorY);

    uint32_t *pixels;
    int pitch;
    if (SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch) == 0)
    {
        auto pixelPitch = pitch / 4;
        console->Visit([&](int x, int y, char character, CharacterAttribute attribute)
        {
            // TODO: Figure out a way to use the dirty character attribute to cut down on 
            auto xFBStart = x * charPixelsWide;
            auto yFBStart = y * charPixelsHigh * pixelPitch;
            auto charXStart = (character % fontCharsWide) * charPixelsWide;
            auto charYStart = (character / fontCharsWide) * charPixelsHigh;
            auto isCursor = (x == cursorX) && (y == cursorY);

            for (uint16_t charLine = 0; charLine < charPixelsHigh; charLine++)
            {
                for (uint16_t charColumn = 0; charColumn < charPixelsWide; charColumn++)
                {
                    auto charValue = fontBuffer[(charYStart + charLine) * fontBufferWidth + (charXStart + charColumn)];
                    if (((attribute & CharacterAttribute::Inverted) == CharacterAttribute::Inverted) ^ (cursorOn && isCursor))
                    {
                        charValue = !charValue;
                    }

                    pixels[(yFBStart + charLine * pixelPitch) + xFBStart + charColumn] = charValue ? foregroundColour : backgroundColour;
                }
            }
        });
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
    else
    {
        fprintf(stderr, "Failed to lock texture (%s)\n", SDL_GetError());
    } 
}
