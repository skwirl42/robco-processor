#include "ConsoleSDLRenderer.h"
#include "Console.h"
#include "emulator.h"
#include "syscall.h"

#if defined(APPLE)
#include <SDL2/SDL.h>
#define OUT_FILE    stderr
#else // defined(APPLE)
#include <SDL.h>
#define OUT_FILE    stdout
#endif // defined(APPLE)
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <filesystem>
#include <boost/program_options.hpp>

#include "syscall_handlers.h"
#include "key_conversion.h"
#include "assembler.h"
#include "opcodes.h"
#include <sound_system.hpp>

namespace po = boost::program_options;

const char *blank_line = "                                                            ";

void handle_key(SDL_Keysym &keysym, emulator &emulator, Console &console)
{
    auto keycode = sdl_keycode_to_console_key(keysym);

    if (keycode != 0)
    {
        handle_keypress_for_syscall(emulator, keycode);
    }
}

void usage(char** argv, po::options_description& options)
{
    std::filesystem::path command_path{ argv[0] };
    std::cout << "Usage: " << command_path.filename().string() << " [options]" << std::endl;
    std::cout << options << std::endl;
}

int main (int argc, char **argv)
{
    std::string font_name{};
    po::options_description cli_options("Allowed options");
    cli_options.add_options()
        ("help,?", "output the help message")
        ("include,I", po::value< std::vector < std::string>>(), "directories to include when assembling source")
        ("device,D", po::value<std::string>(), "the audio device to use")
        ("font,F", po::value<std::string>(&font_name)->default_value("font/robco-termfont.png"), "font image file")
        ("source,S", po::value<std::string>()->required(), "assembly source file to run")
        ;

    po::variables_map variables;
    po::store(po::command_line_parser(argc, argv).options(cli_options).run(), variables);
    po::notify(variables);

    if (variables.count("help") > 0)
    {
        usage(argv, cli_options);
        return 1;
    }

    emulator rcEmulator;
    if (init_emulator(&rcEmulator, ARCH_ORIGINAL) != NO_ERROR)
    {
        fprintf(OUT_FILE, "Emulator error\n");
        return -1;
    }

    //print_opcode_entries();

    const char** paths = new const char* [variables.count("include") + 1];

    for (int i = 0; i < variables.count("include"); i++)
    {
        paths[i] = variables["include"].as<std::vector<std::string>>()[i].c_str();
    }

    paths[variables.count("include")] = 0;

    if (variables.count("source") != 1)
    {
        fprintf(OUT_FILE, "An assembly source file must be provided\n");
        usage(argv, cli_options);
        return -1;
    }

    const char* sample_file = variables["source"].as<std::string>().c_str();

    assembler_data_t *assembled_data;
    assemble(sample_file, paths, nullptr, None, &assembled_data);

    delete[] paths;

    if (get_error_buffer_size(assembled_data) > 0)
    {
        fprintf(OUT_FILE, "%s", get_error_buffer(assembled_data));
        return -1;
    }

    auto apply_result = apply_assembled_data_to_buffer(assembled_data, rcEmulator.memories.data);

    if (apply_result != ASSEMBLER_SUCCESS)
    {
        fprintf(OUT_FILE, "Failed to properly assemble the target %s\n", sample_file);
        return -1;
    }

    auto exec_address_result = get_starting_executable_address(assembled_data, &rcEmulator.PC);

    if (exec_address_result != ASSEMBLER_SUCCESS)
    {
        fprintf(OUT_FILE, "Couldn't get an executable address from the assembled target %s\n", sample_file);
        return -1;
    }
    
    auto result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (result != 0)
    {
        fprintf(OUT_FILE, "Failed to initialize SDL (%s)\n", SDL_GetError());
        return -1;
    }

    sound_system* synthesizer;
    if (variables.count("device") > 0)
    {
        synthesizer = new sound_system(variables["device"].as<std::string>());
    }
    else
    {
        synthesizer = new sound_system();
    }

    if (synthesizer->is_initialized())
    {
        synthesizer->start_worker_thread();
    }
    else
    {
        fprintf(OUT_FILE, "Failed to initialize sound system with error \"%s\"", synthesizer->get_error());
        return -1;
    }

    ConsoleSDLRenderer *renderer = nullptr;
    if (variables.count("font") > 0)
    {
        auto fontfilename = font_name.c_str();
        // Format of the font file is 16 chars wide, 8 chars tall
        renderer = new ConsoleSDLRenderer(fontfilename, 480, 320, 0xFF00FF00, 0xFF000000, 16, 16, 100);
        renderer->Clear();
    }
    else
    {
        fprintf(OUT_FILE, "Missing argument: font filename\n");
        usage(argv, cli_options);
        return -1;
    }

    if (renderer != nullptr)
    {
		Console console(60, 24);
        Console debugConsole(60,24);
        bool done = false;
        bool emulate = true;
        SDL_Event event;
        int frame = 0;
        opcode_entry_t* executed_opcode = nullptr;

        int debugging_lines_start = console.GetHeight() - DEBUGGING_BUFFER_COUNT;
        char *debugging_buffers[DEBUGGING_BUFFER_COUNT]{};
        for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
        {
            debugging_buffers[i] = new char[LINE_BUFFER_SIZE + 1];
        }

        bool debugging = false;
        if (debugging)
        {
            rcEmulator.current_state = DEBUGGING;
        }

        while (!done)
        {
            inst_result_t result = SUCCESS;

            if (debugging)
            {
                get_debug_info(&rcEmulator, debugging_buffers);
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
                    else
                    {
                        handle_key(event.key.keysym, rcEmulator, console);
                    }
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    auto mouse_button_event = event.button;

                    if (mouse_button_event.button == SDL_BUTTON_LEFT)
                    {
                        if (rcEmulator.current_state == DEBUGGING)
                        {
                            rcEmulator.current_state = RUNNING;
                        }

                        debugging = true;
                    }
                }
            }

            bool show_screen_when_debugging = SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_RIGHT);
            if (!show_screen_when_debugging && (debugging || rcEmulator.current_state == DEBUGGING))
            {
                debugConsole.Clear();
                for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
                {
                    debugConsole.PrintLineAt(debugging_buffers[i], 2, debugging_lines_start + i);
                }
                renderer->Render(&debugConsole, frame++);
            }
            else
            {
                if (rcEmulator.graphics_mode.enabled)
                {
                    renderer->Render(&rcEmulator);
                }
                else
                {
                    renderer->Render(&console, frame++);
                }
            }

            if (emulate && emulator_can_execute(&rcEmulator))
            {
                const int max_cycles = 10000;
                int current_cycle = 0;
                do
                {
                    result = execute_instruction(&rcEmulator, &executed_opcode);
                    if (executed_opcode->cycles > 0)
                    {
                        current_cycle += executed_opcode->cycles;
                        auto cycle_time = std::chrono::microseconds(executed_opcode->cycles);
                        auto end_time = cycle_time + std::chrono::high_resolution_clock::now();
                        while (std::chrono::high_resolution_clock::now() < end_time)
                        {
                            // max_cycles microseconds of spinning isn't gonna hurt anything
                        }
                    }
                    else
                    {
                        current_cycle++;
                    }

                    if (debugging)
                    {
                        break;
                    }
                } while (result == SUCCESS && current_cycle <= max_cycles);

                if (result == EXECUTE_SYSCALL)
                {
                    handle_current_syscall(rcEmulator, console, synthesizer);
                }
                else if (result == ILLEGAL_INSTRUCTION)
                {
                    fprintf(OUT_FILE, "Emulation failed with an illegal instruction\n");
                    emulate = false;
                }

                if (debugging && rcEmulator.current_state != WAITING)
                {
                    rcEmulator.current_state = DEBUGGING;
                }
            }
        }

        for (int i = 0; i < DEBUGGING_BUFFER_COUNT; i++)
        {
            delete [] (debugging_buffers[i]);
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