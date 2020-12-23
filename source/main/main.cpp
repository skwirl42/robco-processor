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
#include <boost/algorithm/string/join.hpp>

#include "syscall_handlers.h"
#include "syscall_holotape_handlers.h"
#include "key_conversion.h"
#include "assembler.h"
#include "opcodes.h"
#include "sound_system.hpp"
#include "exceptions.hpp"

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

void conflicting_options(po::variables_map &variables, const char *option1, const char *option2)
{
    if (variables.count(option1) && !variables[option1].defaulted()
        && variables.count(option2) && !variables[option2].defaulted())
    {
        throw std::logic_error(std::string("Option ") + option1 + " cannot be used with option " + option2);
    }
}

void option_dependency(po::variables_map &variables, const char *dependent, const char *required)
{
    if (variables.count(dependent) && !variables[dependent].defaulted())
    {
        if (variables.count(required) == 0 || variables[required].defaulted())
        {
            throw std::logic_error(std::string("Option ") + dependent + " requires option " + required + " to be specified");
        }
    }
}

void one_of_options_required(po::variables_map &variables, std::vector<std::string> &options)
{
    bool any_specified = false;
    for (auto option : options)
    {
        if (variables.count(option) && !variables[option].defaulted())
        {
            any_specified = true;
            break;
        }
    }

    if (!any_specified)
    {
        auto options_list = boost::join(options, ", ");
        throw std::logic_error("One of the following options must be specified: (" + options_list + ")");
    }
}

int main (int argc, char **argv)
{
    std::vector<std::string> one_of_options{"source", "exec-tape"};
    std::string font_name{};
    po::options_description cli_options("Allowed options");
    cli_options.add_options()
        ("help,?", "output the help message")
        ("include,I", po::value< std::vector < std::string>>(), "directories to include when assembling source")
        ("device,D", po::value<std::string>(), "the audio device to use")
        ("font,F", po::value<std::string>(&font_name)->default_value("font/robco-termfont.png"), "font image file")
        ("source,S", po::value<std::string>(), "assembly source file to run")
        ("tape,T", po::value<std::string>(), "a file containing holotape data to be used by the emulator")
        ("exec-tape,X", "execute the first file on the tape provided")
        ;

    po::variables_map variables;
    po::store(po::command_line_parser(argc, argv).options(cli_options).run(), variables);

    emulator rcEmulator;
    ConsoleSDLRenderer *renderer = nullptr;
    bool sdl_initialized = false;
    bool emulator_initialized = false;
    sound_system* synthesizer = nullptr;
    Console console(60, 24);
    Console debugConsole(60,24);

    auto teardown = [&]()
    {
        if (holotape_initialized())
        {
            dispose_holotape();
        }

        if (emulator_initialized)
        {
            dispose_emulator(&rcEmulator);
        }

        if (synthesizer != nullptr)
        {
            delete synthesizer;
        }

        if (renderer != nullptr)
        {
            delete renderer;
        }

        if (sdl_initialized)
        {
            SDL_Quit();
        }
    };

    try
    {
        po::notify(variables);
        conflicting_options(variables, "source", "exec-tape");
        option_dependency(variables, "exec-tape", "tape");
        option_dependency(variables, "include", "source");

        one_of_options_required(variables, one_of_options);

        if (variables.count("help") > 0)
        {
            usage(argv, cli_options);
            return 1;
        }

        if (init_emulator(&rcEmulator, ARCH_ORIGINAL) != NO_ERROR)
        {
            fprintf(OUT_FILE, "Emulator error\n");
            teardown();
            return -1;
        }

        emulator_initialized = true;

        //print_opcode_entries();

        if (variables.count("tape") != 0)
        {
            insert_holotape(variables["tape"].as<std::string>().c_str());
        }

        if (variables.count("source") > 0)
        {
            const char* sample_file = variables["source"].as<std::string>().c_str();

            const char** paths = new const char* [variables.count("include") + 1];

            for (int i = 0; i < variables.count("include"); i++)
            {
                paths[i] = variables["include"].as<std::vector<std::string>>()[i].c_str();
            }

            paths[variables.count("include")] = 0;

            assembler_data_t *assembled_data;
            assemble(sample_file, paths, nullptr, None, &assembled_data);

            delete[] paths;

            if (get_error_buffer_size(assembled_data) > 0)
            {
                fprintf(OUT_FILE, "%s", get_error_buffer(assembled_data));
                teardown();
                return -1;
            }

            auto apply_result = apply_assembled_data_to_buffer(assembled_data, rcEmulator.memories.data);

            if (apply_result != ASSEMBLER_SUCCESS)
            {
                fprintf(OUT_FILE, "Failed to properly assemble the target %s\n", sample_file);
                teardown();
                return -1;
            }

            auto exec_address_result = get_starting_executable_address(assembled_data, &rcEmulator.PC);

            if (exec_address_result != ASSEMBLER_SUCCESS)
            {
                fprintf(OUT_FILE, "Couldn't get an executable address from the assembled target %s\n", sample_file);
                teardown();
                return -1;
            }
        }
        else if (variables.count("exec-tape") > 0)
        {
            if (variables.count("exec-tape") > 0)
            {
                rcEmulator.current_syscall = SYSCALL_EXECUTE;
                handle_current_syscall(rcEmulator, console, synthesizer);
                if (rcEmulator.SP > 0)
                {
                    // The stack should be empty after running the execute syscall, if it succeeded.
                    // This means there was an error executing from tape and the error number is on the stack
                    auto error_code = rcEmulator.memories.user_stack[0];
                    std::cerr << "Failed to execute from tape " << variables["tape"].as<std::string>() << " with error code " << (int)error_code << std::endl;
                    teardown();
                    return -1;
                }
            }
        }
        else
        {
            auto options_list = boost::join(one_of_options, ", ");
            std::cerr << "No executable was found to start the emulator. You must specify one of these options: (" << options_list << ")" << std::endl;
            usage(argv, cli_options);
            teardown();
            return -1;
        }
        
        auto result = SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (result != 0)
        {
            fprintf(OUT_FILE, "Failed to initialize SDL (%s)\n", SDL_GetError());
            teardown();
            return -1;
        }

        sdl_initialized = true;

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
            teardown();
            return -1;
        }

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
            teardown();
            return -1;
        }

        if (renderer != nullptr)
        {
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
    }
    catch (const basic_error& error)
    {
		std::string const* error_text = boost::get_error_info<error_message>(error);
        if (error_text == nullptr)
        {
            std::cerr << "Unknown error occurred" << std::endl;
        }
        else
        {
            std::cerr << "Error: " << *error_text << std::endl;
        }
        teardown();
        return -1;
    }
    catch (po::required_option& exception)
    {
        std::cerr << "Option " << exception.get_option_name() << " is required" << std::endl;
        usage(argv, cli_options);
        teardown();
        return -1;
    }
    catch(const std::logic_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        usage(argv, cli_options);
        teardown();
        return -1;
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        teardown();
        return 0;
    }

    teardown();
    return 0;
}