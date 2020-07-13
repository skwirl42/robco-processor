#pragma once

#include <stdint.h>

class Console;

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class ConsoleSDLRenderer
{
public:
    ConsoleSDLRenderer(const char *fontFilename, int width, int height, uint32_t foregroundColour, uint32_t backgroundColour, int cursorBlinkFrames);

	void Clear();
	void SetColours(uint32_t foregroundColour, uint32_t backgroundColour);

	void Render(Console *console, int frame);

    bool IsValid() { return isValid; }

protected:
    void Cleanup();

private:
    bool *fontBuffer;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int width;
    int height;
    uint32_t foregroundColour;
    uint32_t backgroundColour;
	int cursorBlinkFrames;
    bool isValid;
};