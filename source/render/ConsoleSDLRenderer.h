#pragma once

#include <stdint.h>

class Console;

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
typedef struct _emulator emulator;

class ConsoleSDLRenderer
{
public:
    ConsoleSDLRenderer(const char *fontFilename, int width, int height, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t dimForegroundColour, uint32_t dimBackgroundColour, uint16_t fontCharsWide, uint16_t fontCharsHigh, int cursorBlinkFrames);
    ~ConsoleSDLRenderer();

	void Clear();
	void SetColours(uint32_t foregroundColour, uint32_t backgroundColour);

	void Render(Console *console, int frame);
    void Render(emulator* emulator);

    bool IsValid() { return isValid; }

    int GetCursorBlinkFrames() const { return cursorBlinkFrames; }
    void SetCursorBlinkFrames(int frames) { cursorBlinkFrames = frames; }

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
    uint32_t dimForegroundColour;
    uint32_t dimBackgroundColour;
	uint16_t fontBufferWidth;
	uint16_t fontBufferHeight;
	uint16_t fontCharsWide;
	uint16_t fontCharsHigh;
	uint16_t charPixelsWide;
	uint16_t charPixelsHigh;
	int cursorBlinkFrames;
    bool isValid;
};