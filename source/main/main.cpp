#include "ConsoleSDLRenderer.h"
#include "Console.h"

extern "C"
{
    #include "emulator.h"
    #include "syscall.h"
}

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

uint8_t testCode[] =
{
    0x7f, 0x01, 0x06,
    0x00, 0x48,
    0x00, 0x65,
    0x00, 0x6C,
    0x00, 0x6C,
    0x00, 0x6F,
    0x00, 0x20,
    0x00, 0x77,
    0x00, 0x6F,
    0x00, 0x72,
    0x00, 0x6C,
    0x00, 0x64,
    0x00, 0x21,
    0x00, 0x0D,
    0x00, 0x0A,
    0x20, 0x00, 0x0E,
    0x7f, 0x01, 0x02,
    0x60, 0xFF, 0xDE
};

int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

bool handle_key(SDL_Keycode keycode, Console &console)
{
    bool done = false;
    int consoleWidth = console.GetWidth();
    int consoleHeight = console.GetHeight();
    int cursorX;
    int cursorY;
    console.GetCursor(cursorX, cursorY);
    switch (keycode)
    {
    case SDLK_LEFT:
        cursorX = mod(cursorX - 1, consoleWidth);
        console.SetCursor(cursorX, cursorY);
        break;

    case SDLK_RIGHT:
        cursorX = mod(cursorX + 1, consoleWidth);
        console.SetCursor(cursorX, cursorY);
        break;

    case SDLK_UP:
        cursorY = mod(cursorY - 1, consoleHeight);
        console.SetCursor(cursorX, cursorY);
        break;

    case SDLK_DOWN:
        cursorY = mod(cursorY + 1, consoleHeight);
        console.SetCursor(cursorX, cursorY);
        break;

    case SDLK_ESCAPE:
        done = true;
        break;

    default:
        break;
    }

    return done;
}

typedef union
{
    uint16_t word;
    uint8_t bytes[2];
} emulator_word;

void handle_syscall_print(emulator &emulator, Console &console)
{
    emulator_word stringSize;
    stringSize.bytes[1] = emulator.memories.data[emulator.SP];
    stringSize.bytes[0] = emulator.memories.data[emulator.SP+1];
    char *string = new char[stringSize.word + 1];
    uint16_t stringEnd = emulator.SP + 1;
    uint16_t stringPos = emulator.SP + 1 + stringSize.word;
    for (int x = 0; stringPos > stringEnd; stringPos--, x++)
    {
        string[x] = emulator.memories.data[stringPos];
    }

    string[stringSize.word] = 0;
    console.Print(string);

    delete [] string;
}

void handle_syscall(emulator &emulator, Console &console)
{
    printf("Handling syscall 0x%04x\n", emulator.current_syscall);
    switch (emulator.current_syscall)
    {
    case SYSCALL_CLEAR:
        console.Clear();
        break;

    case SYSCALL_PRINT:
        handle_syscall_print(emulator, console);
        break;

    default:
        break;
    }
}

int main (int argc, char **argv)
{
    emulator rcEmulator;
    if (init_emulator(&rcEmulator, ARCH_ORIGINAL) != NO_ERROR)
    {
        fprintf(stderr, "Emulator error\n");
        return -1;
    }

    memcpy(rcEmulator.memories.instruction, testCode, sizeof(testCode));
    
    auto result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (result != 0)
    {
        fprintf(stderr, "Failed to initialize SDL (%s)\n", SDL_GetError());
        return -1;
    }

    ConsoleSDLRenderer *renderer = nullptr;
    if (argc > 1)
    {
        auto fontfilename = argv[1];
        // Format of the font file is 16 chars wide, 8 chars tall
        renderer = new ConsoleSDLRenderer(fontfilename, 480, 320, 0xFF00FF00, 0xFF000000, 16, 8, 10);
    }
    else
    {
        fprintf(stderr, "Missing argument: font filename\n");
    }

    if (renderer != nullptr)
    {
		Console console(60, 24);
		console.SetCursor(0, 0);
		console.SetCurrentAttribute(CharacterAttribute::Inverted);
		console.PrintLine("Testing the console functionality");
		console.SetCurrentAttribute(CharacterAttribute::None);
		console.PrintLine("THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG");
		console.PrintLine("the quick brown fox jumped over the lazy dog");
		console.PrintLine("01234567890123456789");
		console.PrintLine("!@#$%^&*()_+-=[]{}\\/|,.<>?;:\"'");
		console.PrintLine("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed");
		console.PrintLine("do eiusmod tempor incididunt ut labore et dolore magna");
		console.PrintLine("aliqua. Ut enim ad minim veniam, quis nostrud exercitation");
		console.PrintLine("ullamco laboris nisi ut aliquip ex ea commodo consequat.");
		console.PrintLine("Duis aute irure dolor in reprehenderit in voluptate velit");
		console.PrintLine("esse cillum dolore eu fugiat nulla pariatur. Excepteur sint");
		console.PrintLine("occaecat cupidatat non proident, sunt in culpa qui officia");
		console.PrintLine("deserunt mollit anim id est laborum.");

		int cursorX;
		int cursorY;
		console.GetCursor(cursorX, cursorY);
		console.SetChar(cursorX, cursorY, '*');

        bool done = false;
        bool emulate = true;
        SDL_Event event;
        int frame = 0;
        while (!done)
        {
            if (emulate)
            {
                auto result = execute_instruction(&rcEmulator);
                if (result == EXECUTE_SYSCALL)
                {
                    handle_syscall(rcEmulator, console);
                }
                else if (result == ILLEGAL_INSTRUCTION)
                {
                    fprintf(stderr, "Emulation failed with an illegal instruction\n");
                    emulate = false;
                }
            }

            if (SDL_WaitEventTimeout(&event, 30) != 0)
            {
                if (event.type == SDL_QUIT)
                {
                    done = true;
                }
                else if (event.type == SDL_KEYDOWN)
                {
                    done = handle_key(event.key.keysym.sym, console);
                }
            }

            renderer->Render(&console, frame++);
        }
    }

    if (renderer != nullptr)
    {
        delete renderer;
    }

    SDL_Quit();

    return 0;
}