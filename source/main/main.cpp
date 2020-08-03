#include "ConsoleSDLRenderer.h"
#include "Console.h"
#include "emulator.h"
#include "syscall.h"

#if defined(APPLE)
#include <SDL2/SDL.h>
#else // defined(APPLE)
#include <SDL.h>
#endif // defined(APPLE)
#include <stdio.h>
#include <string.h>

#include "syscall_handlers.h"
#include "key_conversion.h"
#include "assembler.h"
#include "opcodes.h"

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

const char *blank_line = "                                                            ";

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

    const char *paths[] =
    {
        "samples/",
        0
    };

    assembler_data_t *assembled_data;
    // auto assemble_result = assemble("samples/helloworld.asm", paths, &assembled_data);
    auto assemble_result = assemble("samples/data_test.asm", paths, &assembled_data);

    if (assemble_result.error == nullptr || strlen(assemble_result.error) == 0)
    {
        memcpy(rcEmulator.memories.instruction, assembled_data->instruction, assembled_data->instruction_size);
        memcpy(&rcEmulator.memories.data[0x100], assembled_data->data, assembled_data->data_size);
    }
    else
    {
        fprintf(stderr, "Failed to properly assemble the target %s (error: %s)\n", "samples/helloworld.asm", assemble_result.error);
        memcpy(rcEmulator.memories.instruction, testCode, sizeof(testCode));
    }
    
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
        renderer = new ConsoleSDLRenderer(fontfilename, 480, 320, 0xFF00FF00, 0xFF000000, 16, 8, 100);
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

        int debugging_lines_start = console.GetHeight() - DEBUGGING_BUFFER_COUNT;
        char *debugging_buffers[DEBUGGING_BUFFER_COUNT];
        for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
        {
            debugging_buffers[i] = new char[LINE_BUFFER_SIZE + 1];
        }

        bool debugging = false;

        while (!done)
        {
            if (emulate && emulator_can_execute(&rcEmulator))
            {
                auto result = execute_instruction(&rcEmulator, debugging_buffers);
                if (result == EXECUTE_SYSCALL)
                {
                    handle_current_syscall(rcEmulator, console);
                }
                else if (result == ILLEGAL_INSTRUCTION)
                {
                    fprintf(stderr, "Emulation failed with an illegal instruction\n");
                    emulate = false;
                }

                if (debugging)
                {
                    rcEmulator.current_state = DEBUGGING;
                }
            }

            if (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    done = true;
                }
                else if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.sym == SDLK_F5)
                    {
                        debugging = false;
                        if (rcEmulator.current_state == DEBUGGING)
                        {
                            rcEmulator.current_state = RUNNING;
                        }
                    }
                    else if (emulator_can_execute(&rcEmulator))
                    {
                        handle_key(event.key.keysym, rcEmulator, console);
                    }
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    if (rcEmulator.current_state == DEBUGGING)
                    {
                        rcEmulator.current_state = RUNNING;
                        debugging = true;
                    }
                    else
                    {
                        debugging = true;
                    }
                }
            }

            if (debugging || rcEmulator.current_state == DEBUGGING)
            {
                for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
                {
                    console.PrintAt(blank_line, 0, debugging_lines_start + i);
                    console.PrintLineAt(debugging_buffers[i], 2, debugging_lines_start + i);
                }
            }

            renderer->Render(&console, frame++);
        }

        for (int i = 0; i < 4; i++)
        {
            delete [] debugging_buffers[i];
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