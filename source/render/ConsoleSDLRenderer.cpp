#include "ConsoleSDLRenderer.h"

#if APPLE
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#else // APPLE
#include <SDL.h>
#include <SDL_image.h>
#endif // APPLE
#include <stdio.h>

#include "Console.h"
#include "graphics.h"
#include "emulator.h"

namespace
{
    uint32_t lut1[]
    {
        0xFF000000,
        0xFF00FF00
    };

    uint32_t lut2[]
    {
        0xFF000000,
        0xFF005500,
        0xFF00AA00,
        0xFF00FF00
    };

    uint32_t lut4[]
    {
        0xFF000000,
        0xFF001100,
        0xFF002200,
        0xFF002200,
        0xFF003300,
        0xFF004400,
        0xFF005500,
        0xFF006600,
        0xFF007700,
        0xFF008800,
        0xFF009900,
        0xFF00AA00,
        0xFF00BB00,
        0xFF00CC00,
        0xFF00DD00,
        0xFF00EE00,
        0xFF00FF00
    };

    uint32_t lut8[256];
}

ConsoleSDLRenderer::ConsoleSDLRenderer(const char *fontFilename, int width, int height, uint32_t foregroundColour, uint32_t backgroundColour, uint16_t fontCharsWide, uint16_t fontCharsHigh, int cursorBlinkFrames)
    : window(nullptr), renderer(nullptr), texture(nullptr), fontBuffer(nullptr),
      width(width), height(height), foregroundColour(foregroundColour), backgroundColour(backgroundColour),
      fontCharsWide(fontCharsWide), fontCharsHigh(fontCharsHigh),
      cursorBlinkFrames(cursorBlinkFrames),
      charPixelsHigh(0), charPixelsWide(0), fontBufferWidth(0), fontBufferHeight(0),
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

    auto fontBufferSize = (uint64_t)fontSurface->w * fontSurface->h;
    fontBuffer = new bool[fontBufferSize];

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
                fontBuffer[fontIndex] = (bool)(pixels[surfaceIndex] > 0);
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

    for (int i = 0; i < 256; i++)
    {
        uint8_t byte = i & 0xFF;
        lut8[i] = 0xFF000000 | (byte << 8);
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
    this->foregroundColour = foregroundColour;
    this->backgroundColour = backgroundColour;
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
    // TODO: Figure out a way to use the dirty character attribute to cut down on character drawing
    // Maybe use a host-side buffer to write to, then copy it to the texture pixels?
    if (SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch) == 0)
    {
        auto pixelPitch = pitch / 4;
        console->Visit([&](int x, int y, char character, CharacterAttribute attribute)
        {
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

void ConsoleSDLRenderer::Render(emulator* emulator)
{
    if (!isValid) return;
    if (!emulator->graphics_mode.enabled) return;

    uint32_t* pixels;
    int pitch;
    if (SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch) == 0)
    {
        auto pixelPitch = pitch / 4;

        auto emulator_pix_per_line = graphics_pixels_per_line(emulator->graphics_mode);
        int pix_per_emu_pix = 1;
        if (emulator_pix_per_line < 192)
        {
            pix_per_emu_pix = 4;
        }
        else if (emulator_pix_per_line < 320)
        {
            pix_per_emu_pix = 2;
        }

        auto emulator_lines = graphics_get_row_count(emulator->graphics_mode);
        auto emulator_columns = graphics_bytes_per_line(emulator->graphics_mode);
        auto border_width = (width - (emulator_pix_per_line * pix_per_emu_pix)) / 2;
        auto border_height = (height - (emulator_lines * pix_per_emu_pix)) / 2;
        auto emulator_pixels = (pixel_byte_t*)(&emulator->memories.data[emulator->graphics_start]);
        int pixel_divisor = graphics_get_bit_divisor(emulator->graphics_mode);
        //printf("Pix per: %d, lines: %d, border: %dx%d\n", pix_per_emu_pix, emulator_lines, border_width, border_height);

        for (int y = 0; y < height; y++)
        {
            bool inYBorder = (y < border_height) || ((height - y) < border_height);
            for (int x = 0; x < width; x++)
            {
                bool inXBorder = ((x < border_width) || ((width - x) < border_width));

                if (inYBorder || inXBorder)
                {
                    pixels[(y * pixelPitch) + x] = emulator->graphics_mode.border ? foregroundColour : backgroundColour;
                }
                else
                {
                    int emuX = (x - border_width) / pix_per_emu_pix;
                    int emuY = (y - border_height) / pix_per_emu_pix;
                    uint32_t pixelValue = backgroundColour;
                    int byte = (emuY * emulator_columns) + (emuX / pixel_divisor);
                    auto pixel_byte = emulator_pixels[byte];
                    int bit = emuX & 0b111;
                    int two_bits = emuX & 0b11;
                    int nybble = emuX & 1;
                    switch (emulator->graphics_mode.depth)
                    {
                    case ONE_BIT_PER_PIXEL:
                        switch (bit)
                        {
                        case 0:
                            pixelValue = lut1[pixel_byte.one_bit.pixel0];
                            break;
                        case 1:
                            pixelValue = lut1[pixel_byte.one_bit.pixel1];
                            break;
                        case 2:
                            pixelValue = lut1[pixel_byte.one_bit.pixel2];
                            break;
                        case 3:
                            pixelValue = lut1[pixel_byte.one_bit.pixel3];
                            break;
                        case 4:
                            pixelValue = lut1[pixel_byte.one_bit.pixel4];
                            break;
                        case 5:
                            pixelValue = lut1[pixel_byte.one_bit.pixel5];
                            break;
                        case 6:
                            pixelValue = lut1[pixel_byte.one_bit.pixel6];
                            break;
                        case 7:
                            pixelValue = lut1[pixel_byte.one_bit.pixel7];
                            break;
                        }
                        break;

                    case TWO_BITS_PER_PIXEL:
                        switch (two_bits)
                        {
                        case 0:
                            pixelValue = lut2[pixel_byte.two_bit.pixel0];
                            break;
                        case 1:
                            pixelValue = lut2[pixel_byte.two_bit.pixel1];
                            break;
                        case 2:
                            pixelValue = lut2[pixel_byte.two_bit.pixel2];
                            break;
                        case 3:
                            pixelValue = lut2[pixel_byte.two_bit.pixel3];
                            break;
                        }
                        break;

                    case FOUR_BITS_PER_PIXEL:
                        pixelValue = nybble ? lut4[pixel_byte.four_bit.pixel1] : lut4[pixel_byte.four_bit.pixel0];
                        break;

                    case EIGHT_BITS_PER_PIXEL:
                        pixelValue = lut8[pixel_byte.eight_bit];
                        break;
                    }
                    pixels[(y * pixelPitch) + x] = pixelValue;
                }
            }
        }

        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
    else
    {
        fprintf(stderr, "Failed to lock texture (%s)\n", SDL_GetError());
    }
}