#include "ConsoleSDLRenderer.h"
#include "Console.h"
#include "emulator.h"
#include "syscall.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "syscall_handlers.h"
#include "key_conversion.h"

uint8_t testCode[] =
{
    // start:
    0x7f, 0x01, 0x06, // syscall clear
    // loop:
    0x00, 0x48, // push H
    0x00, 0x65, // push e
    0x00, 0x6C, // push l
    0x00, 0x6C, // push l
    0x00, 0x6F, // push o
    0x00, 0x20, // push ' '
    0x00, 0x77, // push w
    0x00, 0x6F, // push o
    0x00, 0x72, // push r
    0x00, 0x6C, // push l
    0x00, 0x64, // push d
    0x00, 0x21, // push !
    0x00, 0x0D, // push \r
    0x00, 0x0A, // push \n
    0x20, 0x00, 0x0E, // push 14
    0x7f, 0x01, 0x02, // syscall print
    0x60, 0xFF, 0xDE  // b loop
};

int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

void handle_key(SDL_Keysym &keysym, emulator &emulator, Console &console)
{
    auto keycode = sdl_keycode_to_console_key(keysym);

    if (keycode != 0)
    {
        handle_keypress_for_syscall(emulator, keycode);
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
        renderer->Clear();
    }
    else
    {
        fprintf(stderr, "Missing argument: font filename\n");
    }

    if (renderer != nullptr)
    {
		Console console(60, 24);
        bool done = false;
        bool emulate = true;
        SDL_Event event;
        int frame = 0;
        while (!done)
        {
            if (emulate && emulator_can_execute(&rcEmulator))
            {
                auto result = execute_instruction(&rcEmulator);
                if (result == EXECUTE_SYSCALL)
                {
                    handle_current_syscall(rcEmulator, console);
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
                    handle_key(event.key.keysym, rcEmulator, console);
                }
            }

            renderer->Render(&console, frame++);
        }
    }

    dispose_emulator(&rcEmulator);

    if (renderer != nullptr)
    {
        delete renderer;
    }

    SDL_Quit();

    return 0;
}